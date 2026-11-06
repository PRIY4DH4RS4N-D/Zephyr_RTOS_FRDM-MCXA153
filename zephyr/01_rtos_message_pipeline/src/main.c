/* Static queues carry values through three independently scheduled stages. */
#include "app.h"

struct message {
  uint32_t id;
  int32_t value;
  bool test;
};
struct result {
  uint32_t id;
  int32_t input;
  int32_t doubled;
  bool test;
};
K_MSGQ_DEFINE(input_q, sizeof(struct message), 8, 4);
K_MSGQ_DEFINE(output_q, sizeof(struct result), 8, 4);
K_MUTEX_DEFINE(metrics_lock);
K_SEM_DEFINE(producer_wake, 0, 1);
K_SEM_DEFINE(worker_ready, 0, 1);
K_SEM_DEFINE(test_done, 0, 1);
static atomic_t running, sequence;
static uint32_t accepted, processed, logged, overflow;

static int inject(int32_t value, bool test) {
  struct message msg = {
      .id = (uint32_t)atomic_inc(&sequence) + 1, .value = value, .test = test};
  int rc = k_msgq_put(&input_q, &msg, K_NO_WAIT);
  k_mutex_lock(&metrics_lock, K_FOREVER);
  if (rc)
    overflow++;
  else
    accepted++;
  k_mutex_unlock(&metrics_lock);
  report(rc ? "OVERFLOW" : "QUEUED", "id=%u value=%d rc=%d", msg.id, value, rc);
  return rc;
}

static void producer(void *a, void *b, void *c) {
  ARG_UNUSED(a);
  ARG_UNUSED(b);
  ARG_UNUSED(c);
  int32_t value = 0;
  for (;;) {
    k_sem_take(&producer_wake, K_FOREVER);
    while (atomic_get(&running)) {
      inject(value++, false);
      if (value > 1000)
        value = 0;
      k_sleep(K_MSEC(500));
    }
  }
}

static void worker(void *a, void *b, void *c) {
  ARG_UNUSED(a);
  ARG_UNUSED(b);
  ARG_UNUSED(c);
  k_sem_take(&worker_ready, K_FOREVER);
  struct message msg;
  for (;;) {
    int rc = k_msgq_get(&input_q, &msg, K_FOREVER);
    if (rc) {
      report("FAIL", "input queue rc=%d", rc);
      continue;
    }
    struct result out = {.id = msg.id,
                         .input = msg.value,
                         .doubled = msg.value * 2,
                         .test = msg.test};
    k_mutex_lock(&metrics_lock, K_FOREVER);
    processed++;
    k_mutex_unlock(&metrics_lock);
    /* Backpressure blocks this noncritical worker without spinning or losing
     * results. */
    rc = k_msgq_put(&output_q, &out, K_FOREVER);
    if (rc)
      report("FAIL", "result queue rc=%d", rc);
  }
}

static void logger(void *a, void *b, void *c) {
  ARG_UNUSED(a);
  ARG_UNUSED(b);
  ARG_UNUSED(c);
  struct result out;
  for (;;) {
    k_msgq_get(&output_q, &out, K_FOREVER);
    report("RESULT", "id=%u input=%d doubled=%d", out.id, out.input,
           out.doubled);
    k_mutex_lock(&metrics_lock, K_FOREVER);
    logged++;
    k_mutex_unlock(&metrics_lock);
    if (out.test) {
      check("123 traversed pipeline and became 246",
            out.input == 123 && out.doubled == 246);
      k_sem_give(&test_done);
    }
  }
}

K_THREAD_DEFINE(producer_id, 1024, producer, NULL, NULL, NULL, 6, 0, 0);
K_THREAD_DEFINE(worker_id, 1024, worker, NULL, NULL, NULL, 4, 0, 0);
K_THREAD_DEFINE(logger_id, 1280, logger, NULL, NULL, NULL, 7, 0, 0);

static int cmd_start(const struct shell *sh, size_t argc, char **argv) {
  ARG_UNUSED(sh);
  ARG_UNUSED(argc);
  ARG_UNUSED(argv);
  atomic_set(&running, 1);
  k_sem_give(&producer_wake);
  report("CONFIG", "producer started");
  return 0;
}
static int cmd_stop(const struct shell *sh, size_t argc, char **argv) {
  ARG_UNUSED(sh);
  ARG_UNUSED(argc);
  ARG_UNUSED(argv);
  atomic_clear(&running);
  report("CONFIG", "producer stopped; queued messages will drain");
  return 0;
}
static int cmd_inject(const struct shell *sh, size_t argc, char **argv) {
  ARG_UNUSED(sh);
  ARG_UNUSED(argc);
  long value;
  if (number(argv[1], -1000000, 1000000, &value))
    return -EINVAL;
  return inject(value, false);
}
static int cmd_status(const struct shell *sh, size_t argc, char **argv) {
  ARG_UNUSED(sh);
  ARG_UNUSED(argc);
  ARG_UNUSED(argv);
  k_mutex_lock(&metrics_lock, K_FOREVER);
  report("STATUS",
         "running=%ld accepted=%u processed=%u logged=%u overflow=%u "
         "input=%u/8 output=%u/8",
         (long)atomic_get(&running), accepted, processed, logged, overflow,
         k_msgq_num_used_get(&input_q), k_msgq_num_used_get(&output_q));
  k_mutex_unlock(&metrics_lock);
  return 0;
}
SHELL_STATIC_SUBCMD_SET_CREATE(
    pipeline_cmds,
    SHELL_CMD_ARG(start, NULL, "Start 500 ms producer", cmd_start, 1, 0),
    SHELL_CMD_ARG(stop, NULL, "Stop producer, drain queues", cmd_stop, 1, 0),
    SHELL_CMD_ARG(inject, NULL, "inject <-1000000..1000000>", cmd_inject, 2, 0),
    SHELL_CMD_ARG(status, NULL, "Show queue and transaction counters",
                  cmd_status, 1, 0),
    SHELL_SUBCMD_SET_END);
SHELL_CMD_REGISTER(pipeline, &pipeline_cmds, "RTOS message pipeline", NULL);

int main(void) {
  report("BOOT",
         "Message pipeline ready; type help; producer initially stopped");
  if (IS_ENABLED(CONFIG_APP_AUTOTEST)) {
    /* Worker is gated, so overflow testing is independent of scheduling luck.
     */
    check("Test transaction enqueued", inject(123, true) == 0);
    for (int i = 0; i < 7; i++)
      check("Queue fill", inject(i, false) == 0);
    check("Full queue rejects ninth message", inject(999, false) != 0);
  }
  k_sem_give(&worker_ready);
  if (IS_ENABLED(CONFIG_APP_AUTOTEST)) {
    check("Pipeline test completed within 2 s",
          k_sem_take(&test_done, K_SECONDS(2)) == 0);
  }
  return 0;
}
