/* Task watchdog channels supervise two workers and the supervisor itself.
 * Recovery callbacks only publish flags from timer/ISR context. An explicit
 * reset test stops all feeds and the task timer, leaving WWDT to reset SoC. */
#include "app.h"
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/task_wdt/task_wdt.h>

#define WDT_NODE DT_ALIAS(watchdog0)
BUILD_ASSERT(DT_NODE_HAS_STATUS(WDT_NODE, okay), "Hardware watchdog required");
static const struct device *const hardware_wdt = DEVICE_DT_GET(WDT_NODE);
K_SEM_DEFINE(workers_ready, 0, 2);
K_SEM_DEFINE(supervisor_ready, 0, 1);
static atomic_t fault_mask, expired_mask, reset_requested, reset_latched;
static atomic_t heartbeats[2], last_ms[2], recoveries, ready;
static int channels[3] = {-1, -1, -1};
static uint32_t boot_cause;

static void expired(int channel, void *user) {
  ARG_UNUSED(channel);
  unsigned index = (unsigned)(uintptr_t)user;
  /* Even if the supervisor itself stops, healthy workers must cease feeding
   * once a reset-mode timeout has fired. This callback never logs or blocks. */
  if (index == 2 || atomic_get(&reset_requested))
    atomic_set(&reset_latched, 1);
  atomic_or(&expired_mask, BIT(index));
}
static void worker(void *a, void *b, void *c) {
  ARG_UNUSED(b);
  ARG_UNUSED(c);
  unsigned i = (unsigned)(uintptr_t)a;
  k_sem_take(&workers_ready, K_FOREVER);
  for (;;) {
    if (!atomic_get(&reset_latched) && !(atomic_get(&fault_mask) & BIT(i))) {
      atomic_inc(&heartbeats[i]);
      atomic_set(&last_ms[i], (atomic_val_t)k_uptime_get_32());
      int rc = task_wdt_feed(channels[i]);
      if (rc)
        report("FAIL", "worker=%u feed_rc=%d", i, rc);
    }
    k_sleep(K_MSEC(i == 0 ? 100 : 200));
  }
}
static bool healthy(unsigned i) {
  return atomic_get(&heartbeats[i]) > 0 &&
         (uint32_t)(k_uptime_get_32() - (uint32_t)atomic_get(&last_ms[i])) <
             600;
}
static void supervisor(void *a, void *b, void *c) {
  ARG_UNUSED(a);
  ARG_UNUSED(b);
  ARG_UNUSED(c);
  k_sem_take(&supervisor_ready, K_FOREVER);
  for (;;) {
    atomic_val_t expired_bits = atomic_set(&expired_mask, 0);
    if (expired_bits) {
      if (atomic_get(&reset_requested) || (expired_bits & BIT(2))) {
        /* Higher priority than workers: latch first, then stop background
         * feeds. task_wdt_suspend() performs one final hardware feed. No
         * software reset. */
        atomic_set(&reset_latched, 1);
        task_wdt_suspend();
        rgb_set(1);
        report("FAULT",
               "expired_mask=0x%lx WWDT reset test latched; all task feeds "
               "stopped; waiting for actual hardware reset",
               (unsigned long)expired_bits);
      } else {
        for (unsigned i = 0; i < 2; i++)
          if (expired_bits & BIT(i)) {
            atomic_and(&fault_mask, ~BIT(i));
            atomic_inc(&recoveries);
            int rc = task_wdt_feed(channels[i]);
            report(rc ? "FAIL" : "RECOVERY",
                   "worker=%u timeout detected; fault cleared; feed_rc=%d", i,
                   rc);
          }
      }
    }
    if (!atomic_get(&reset_latched)) {
      int rc = task_wdt_feed(channels[2]);
      if (rc)
        report("FAIL", "supervisor feed_rc=%d", rc);
      rc = rgb_set(healthy(0) && healthy(1) ? 2 : 3);
      if (rc)
        report("FAIL", "RGB rc=%d", rc);
    }
    k_sleep(K_MSEC(100));
  }
}
K_THREAD_DEFINE(worker0_id, 1024, worker, (void *)0, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(worker1_id, 1024, worker, (void *)1, NULL, NULL, 6, 0, 0);
K_THREAD_DEFINE(supervisor_id, 1536, supervisor, NULL, NULL, NULL, 3, 0, 0);

static int cmd_status(const struct shell *sh, size_t argc, char **argv) {
  ARG_UNUSED(sh);
  ARG_UNUSED(argc);
  ARG_UNUSED(argv);
  report("GUARD",
         "ready=%ld heartbeat0=%ld heartbeat1=%ld healthy0=%d healthy1=%d "
         "fault_mask=0x%lx recoveries=%ld reset_test=%ld reset_latched=%ld "
         "boot_cause=0x%x",
         (long)atomic_get(&ready), (long)atomic_get(&heartbeats[0]),
         (long)atomic_get(&heartbeats[1]), healthy(0), healthy(1),
         (unsigned long)atomic_get(&fault_mask), (long)atomic_get(&recoveries),
         (long)atomic_get(&reset_requested), (long)atomic_get(&reset_latched),
         boot_cause);
  return 0;
}
static int cmd_fault(const struct shell *sh, size_t argc, char **argv) {
  ARG_UNUSED(sh);
  long i;
  if (number(argv[1], 0, 1, &i))
    return -EINVAL;
  bool reset = argc == 3 && !strcmp(argv[2], "reset");
  if (argc == 3 && !reset && strcmp(argv[2], "recover"))
    return -EINVAL;
  if (!atomic_get(&ready))
    return -EAGAIN;
  if (atomic_get(&reset_latched))
    return -EPERM;
  if (atomic_get(&fault_mask))
    return -EBUSY;
  atomic_set(&reset_requested, reset);
  atomic_or(&fault_mask, BIT(i));
  report("FAULT", "worker=%ld mode=%s; timeout=1200ms", i,
         reset ? "ACTUAL WWDT RESET TEST" : "RECOVER");
  return 0;
}
static int cmd_clear(const struct shell *sh, size_t argc, char **argv) {
  ARG_UNUSED(sh);
  ARG_UNUSED(argc);
  ARG_UNUSED(argv);
  if (atomic_get(&reset_latched))
    return -EPERM;
  atomic_clear(&fault_mask);
  atomic_clear(&reset_requested);
  report("GUARD", "injected faults cleared");
  return 0;
}
SHELL_STATIC_SUBCMD_SET_CREATE(
    guard_cmds,
    SHELL_CMD_ARG(status, NULL, "Heartbeats, reset cause, faults", cmd_status,
                  1, 0),
    SHELL_CMD_ARG(
        fault, NULL,
        "fault <0|1> [recover|reset]; reset explicitly causes hardware reboot",
        cmd_fault, 2, 1),
    SHELL_CMD_ARG(clear, NULL, "Clear before reset test latches", cmd_clear, 1,
                  0),
    SHELL_SUBCMD_SET_END);
SHELL_CMD_REGISTER(guard, &guard_cmds, "Task watchdog supervisor", NULL);

int main(void) {
  int rc = rgb_init();
  check("RGB initialization", rc == 0);
  if (rc)
    return rc;
  rc = hwinfo_get_reset_cause(&boot_cause);
  if (!rc) {
    report("BOOT", "reset_cause=0x%x watchdog_bit=%d", boot_cause,
           !!(boot_cause & RESET_WATCHDOG));
    if (boot_cause & RESET_WATCHDOG)
      report("OBSERVED", "hardware watchdog reset cause reported by CMC");
    rc = hwinfo_clear_reset_cause();
    if (rc)
      report("FAIL", "clear reset cause rc=%d", rc);
  } else
    report("UNSUPPORTED", "reset cause rc=%d", rc);
  if (!device_is_ready(hardware_wdt)) {
    report("FAIL", "WWDT device not ready");
    return -ENODEV;
  }
  rc = task_wdt_init(hardware_wdt);
  check("Task watchdog with hardware fallback initialized", rc == 0);
  if (rc)
    return rc;
  for (unsigned i = 0; i < 3; i++) {
    channels[i] =
        task_wdt_add(i < 2 ? 1200 : 2000, expired, (void *)(uintptr_t)i);
    if (channels[i] < 0) {
      report("FAIL", "channel=%u add_rc=%d", i, channels[i]);
      for (unsigned j = 0; j < i; j++)
        task_wdt_delete(channels[j]);
      task_wdt_suspend();
      wdt_disable(hardware_wdt);
      return channels[i];
    }
  }
  atomic_set(&ready, 1);
  k_sem_give(&supervisor_ready);
  k_sem_give(&workers_ready);
  k_sem_give(&workers_ready);
  report("BOOT", "Task watchdog running; normal startup never injects a reset "
                 "fault; type help");
  if (IS_ENABLED(CONFIG_APP_AUTOTEST)) {
    k_sleep(K_SECONDS(1));
    check("Both workers healthy", healthy(0) && healthy(1));
    /* Startup tests exercise healthy operation only. All fault injection is
     * manual. */
    check("No automatic reset/fault injection",
          !atomic_get(&fault_mask) && !atomic_get(&reset_latched));
    cmd_status(NULL, 0, NULL);
  }
  return 0;
}
