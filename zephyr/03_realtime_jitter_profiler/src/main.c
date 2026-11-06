/* Measures signed inter-release jitter with the Cortex-M timing backend.
 * Absolute kernel deadlines prevent drift. No UART in the measured thread. */
#include "app.h"
#include <limits.h>
#include <zephyr/timing/timing.h>

struct measurements {
  uint32_t samples, missed;
  int64_t min_ns, max_ns, sum_ns;
  uint64_t absolute_sum_ns;
};
static struct measurements stats;
static struct k_spinlock data_lock;
static atomic_t active, period_ms = 50, generation, load_us;
K_SEM_DEFINE(start_bench, 0, 1);

static void record(int64_t jitter, uint32_t missed) {
  k_spinlock_key_t key = k_spin_lock(&data_lock);
  if (stats.samples < 1000000) {
    if (stats.samples == 0 || jitter < stats.min_ns)
      stats.min_ns = jitter;
    if (stats.samples == 0 || jitter > stats.max_ns)
      stats.max_ns = jitter;
    stats.samples++;
    stats.missed += missed;
    stats.sum_ns += jitter;
    stats.absolute_sum_ns += jitter < 0 ? (uint64_t)-jitter : (uint64_t)jitter;
  }
  k_spin_unlock(&data_lock, key);
}
static void sampler(void *a, void *b, void *c) {
  ARG_UNUSED(a);
  ARG_UNUSED(b);
  ARG_UNUSED(c);
  for (;;) {
    k_sem_take(&start_bench, K_FOREVER);
    int epoch = atomic_get(&generation);
    int32_t period = atomic_get(&period_ms);
    int64_t tick_period = k_ms_to_ticks_ceil64(period);
    int64_t next = k_uptime_ticks() + tick_period;
    timing_t before = timing_counter_get();
    bool baseline = true;
    while (atomic_get(&active) && epoch == atomic_get(&generation)) {
      k_sleep(K_TIMEOUT_ABS_TICKS(next));
      timing_t after = timing_counter_get();
      int64_t now = k_uptime_ticks();
      if (!atomic_get(&active) || epoch != atomic_get(&generation))
        break;
      uint64_t elapsed =
          timing_cycles_to_ns(timing_cycles_get(&before, &after));
      uint32_t missed = now > next ? (uint32_t)((now - next) / tick_period) : 0;
      /* First interval establishes a release-aligned timing baseline. */
      if (!baseline)
        record((int64_t)elapsed - (int64_t)period * 1000000, missed);
      baseline = false;
      before = after;
      next += (1 + (int64_t)missed) * tick_period;
    }
  }
}
static volatile uint32_t load_sink;
static void workload(void *a, void *b, void *c) {
  ARG_UNUSED(a);
  ARG_UNUSED(b);
  ARG_UNUSED(c);
  for (;;) {
    int us = atomic_get(&load_us);
    if (us) {
      uint32_t begin = k_cycle_get_32();
      uint32_t duration = k_us_to_cyc_floor32(us);
      do {
        load_sink = load_sink * 1664525u + 1013904223u;
      } while ((uint32_t)(k_cycle_get_32() - begin) < duration);
    }
    k_sleep(K_MSEC(10));
  }
}
K_THREAD_DEFINE(sampler_id, 1024, sampler, NULL, NULL, NULL, 2, 0, 0);
K_THREAD_DEFINE(load_id, 768, workload, NULL, NULL, NULL, 8, 0, 0);

static int cmd_start(const struct shell *sh, size_t argc, char **argv) {
  ARG_UNUSED(sh);
  ARG_UNUSED(argc);
  ARG_UNUSED(argv);
  if (atomic_get(&active))
    return -EALREADY;
  k_spinlock_key_t key = k_spin_lock(&data_lock);
  memset(&stats, 0, sizeof(stats));
  k_spin_unlock(&data_lock, key);
  atomic_inc(&generation);
  atomic_set(&active, 1);
  k_sem_give(&start_bench);
  report("BENCH", "started period_ms=%ld timing_hz=%llu",
         (long)atomic_get(&period_ms), timing_freq_get());
  return 0;
}
static int cmd_stop(const struct shell *sh, size_t argc, char **argv) {
  ARG_UNUSED(sh);
  ARG_UNUSED(argc);
  ARG_UNUSED(argv);
  atomic_clear(&active);
  atomic_inc(&generation);
  report("BENCH", "stopped");
  return 0;
}
static int cmd_report(const struct shell *sh, size_t argc, char **argv) {
  ARG_UNUSED(sh);
  ARG_UNUSED(argc);
  ARG_UNUSED(argv);
  k_spinlock_key_t key = k_spin_lock(&data_lock);
  struct measurements s = stats;
  k_spin_unlock(&data_lock, key);
  report("BENCH", "samples=%u missed_periods=%u running=%ld", s.samples,
         s.missed, (long)atomic_get(&active));
  if (s.samples)
    report("JITTER", "min_ns=%lld max_ns=%lld mean_ns=%lld mean_abs_ns=%llu",
           s.min_ns, s.max_ns, s.sum_ns / s.samples,
           s.absolute_sum_ns / s.samples);
  else
    report("JITTER", "no measured samples yet");
  return 0;
}
static int cmd_period(const struct shell *sh, size_t argc, char **argv) {
  ARG_UNUSED(sh);
  ARG_UNUSED(argc);
  long p;
  if (number(argv[1], 5, 1000, &p))
    return -EINVAL;
  if (atomic_get(&active))
    return -EBUSY;
  atomic_set(&period_ms, p);
  report("CONFIG", "period_ms=%ld", p);
  return 0;
}
static int cmd_load(const struct shell *sh, size_t argc, char **argv) {
  ARG_UNUSED(sh);
  ARG_UNUSED(argc);
  long v;
  if (number(argv[1], 0, 20000, &v))
    return -EINVAL;
  atomic_set(&load_us, v);
  report("CONFIG", "low priority work_us=%ld per 10ms sleep", v);
  return 0;
}
SHELL_STATIC_SUBCMD_SET_CREATE(
    bench_cmds,
    SHELL_CMD_ARG(start, NULL, "Start a fresh run", cmd_start, 1, 0),
    SHELL_CMD_ARG(stop, NULL, "Stop sampling", cmd_stop, 1, 0),
    SHELL_CMD_ARG(report, NULL, "Measured inter-release jitter", cmd_report, 1,
                  0),
    SHELL_CMD_ARG(period, NULL, "period <5..1000 ms>; stop first", cmd_period,
                  2, 0),
    SHELL_CMD_ARG(load, NULL, "load <0..20000 us> low priority work", cmd_load,
                  2, 0),
    SHELL_SUBCMD_SET_END);
SHELL_CMD_REGISTER(bench, &bench_cmds, "Real-time jitter profiler", NULL);

int main(void) {
  timing_init();
  timing_start();
  report("BOOT", "Jitter profiler timing_hz=%llu; type help",
         timing_freq_get());
  if (IS_ENABLED(CONFIG_APP_AUTOTEST)) {
    record(-100, 0);
    record(200, 1);
    check("Statistics arithmetic",
          stats.samples == 2 && stats.min_ns == -100 && stats.max_ns == 200 &&
              stats.sum_ns == 100 && stats.missed == 1);
    cmd_start(NULL, 0, NULL);
    k_sleep(K_SECONDS(2));
    cmd_stop(NULL, 0, NULL);
    k_spinlock_key_t key = k_spin_lock(&data_lock);
    uint32_t n = stats.samples;
    k_spin_unlock(&data_lock, key);
    check("Periodic sampler progresses", n >= 20);
    cmd_report(NULL, 0, NULL);
  }
  return 0;
}
