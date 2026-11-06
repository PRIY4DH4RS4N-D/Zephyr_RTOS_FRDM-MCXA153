#include "app.h"
#include <zephyr/drivers/sensor.h>

#define SENSOR_NODE DT_ALIAS(ambient_temp0)
BUILD_ASSERT(DT_NODE_HAS_STATUS(SENSOR_NODE, okay),
             "Onboard ambient temperature sensor required");
BUILD_ASSERT(DT_NODE_HAS_COMPAT(SENSOR_NODE, nxp_p3t1755),
             "Expected onboard P3T1755");
static const struct device *const sensor = DEVICE_DT_GET(SENSOR_NODE);
enum thermal_state { NORMAL, WARNING, CRITICAL, SENSOR_ERROR };
static const char *const labels[] = {"NORMAL", "WARNING", "CRITICAL",
                                     "SENSOR_ERROR"};
static const unsigned colors[] = {2, 3, 1, 4};
K_MUTEX_DEFINE(data_lock);
K_SEM_DEFINE(read_now, 0, 1);
K_SEM_DEFINE(acquire_ready, 0, 1);
static int warn_centi = 3500, critical_centi = 4500;
static int32_t history[4], sum, last_centi, average_centi;
static unsigned used, index;
static uint32_t samples, errors;
static enum thermal_state state = SENSOR_ERROR;
static bool simulated, valid, stream = true;
static int32_t simulated_centi = 2500;
static int64_t last_time;
static atomic_t force_read;

static enum thermal_state classify(int32_t temp, int32_t warning,
                                   int32_t critical) {
  if (temp >= critical)
    return CRITICAL;
  return temp >= warning ? WARNING : NORMAL;
}
static void clear_average(void) {
  memset(history, 0, sizeof(history));
  used = 0;
  index = 0;
  sum = 0;
  valid = false;
}
static void acquisition(void *a, void *b, void *c) {
  ARG_UNUSED(a);
  ARG_UNUSED(b);
  ARG_UNUSED(c);
  k_sem_take(&acquire_ready, K_FOREVER);
  for (;;) {
    /* Configuration and acquisition share a lock so source changes cannot
     * label a real sample as simulated or mix the two moving averages. */
    k_mutex_lock(&data_lock, K_FOREVER);
    int rc = 0;
    int32_t temp = simulated_centi;
    const char *source = simulated ? "SIMULATED" : "SENSOR";
    if (!simulated) {
      struct sensor_value value;
      if (!device_is_ready(sensor))
        rc = -ENODEV;
      else {
        rc = sensor_sample_fetch(sensor);
        if (!rc)
          rc = sensor_channel_get(sensor, SENSOR_CHAN_AMBIENT_TEMP, &value);
      }
      if (!rc)
        temp = value.val1 * 100 + value.val2 / 10000;
    }
    bool requested = atomic_set(&force_read, 0) != 0;
    if (rc) {
      errors++;
      valid = false;
      state = SENSOR_ERROR;
      /* Invalid readings never enter the filter or appear as temperatures. */
      used = 0;
      index = 0;
      sum = 0;
      memset(history, 0, sizeof(history));
      report("FAIL",
             "source=SENSOR read_rc=%d errors=%u no temperature reported", rc,
             errors);
    } else {
      sum -= history[index];
      history[index] = temp;
      sum += temp;
      index = (index + 1) % ARRAY_SIZE(history);
      if (used < ARRAY_SIZE(history))
        used++;
      last_centi = temp;
      average_centi = sum / (int32_t)used;
      samples++;
      last_time = k_uptime_get();
      valid = true;
      /* Raw sample catches rapid excursions; moving average slows recovery. */
      state =
          classify(MAX(last_centi, average_centi), warn_centi, critical_centi);
      if (stream || requested)
        report("THERMAL",
               "source=%s temperature_centiC=%d average_centiC=%d state=%s "
               "sample=%u",
               source, temp, average_centi, labels[state], samples);
    }
    rc = rgb_set(colors[state]);
    if (rc)
      report("FAIL", "RGB rc=%d", rc);
    k_mutex_unlock(&data_lock);
    k_sem_take(&read_now, K_SECONDS(1));
  }
}
K_THREAD_DEFINE(sensor_thread, 1536, acquisition, NULL, NULL, NULL, 5, 0, 0);

static int cmd_read(const struct shell *sh, size_t argc, char **argv) {
  ARG_UNUSED(sh);
  ARG_UNUSED(argc);
  ARG_UNUSED(argv);
  atomic_set(&force_read, 1);
  k_sem_give(&read_now);
  report("REQUEST", "one acquisition requested");
  return 0;
}
static int cmd_threshold(const struct shell *sh, size_t argc, char **argv) {
  ARG_UNUSED(sh);
  ARG_UNUSED(argc);
  long w, c;
  if (number(argv[1], -40, 124, &w) || number(argv[2], -39, 125, &c) || w >= c)
    return -EINVAL;
  k_mutex_lock(&data_lock, K_FOREVER);
  warn_centi = w * 100;
  critical_centi = c * 100;
  k_mutex_unlock(&data_lock);
  k_sem_give(&read_now);
  report("CONFIG", "warning_C=%ld critical_C=%ld", w, c);
  return 0;
}
static int cmd_status(const struct shell *sh, size_t argc, char **argv) {
  ARG_UNUSED(sh);
  ARG_UNUSED(argc);
  ARG_UNUSED(argv);
  k_mutex_lock(&data_lock, K_FOREVER);
  report("STATUS",
         "device_ready=%d source=%s valid=%d state=%s samples=%u errors=%u "
         "warn_centiC=%d critical_centiC=%d",
         device_is_ready(sensor), simulated ? "SIMULATED" : "SENSOR", valid,
         labels[state], samples, errors, warn_centi, critical_centi);
  if (valid)
    report("LATEST", "temperature_centiC=%d average_centiC=%d age_ms=%lld",
           last_centi, average_centi, k_uptime_get() - last_time);
  k_mutex_unlock(&data_lock);
  return 0;
}
static int cmd_simulate(const struct shell *sh, size_t argc, char **argv) {
  ARG_UNUSED(sh);
  ARG_UNUSED(argc);
  long t = 0;
  bool off = !strcmp(argv[1], "off");
  if (!off && number(argv[1], -40, 125, &t))
    return -EINVAL;
  k_mutex_lock(&data_lock, K_FOREVER);
  simulated = !off;
  simulated_centi = t * 100;
  clear_average();
  k_mutex_unlock(&data_lock);
  k_sem_give(&read_now);
  report("CONFIG", "source=%s",
         off ? "SENSOR" : "SIMULATED (test values only)");
  return 0;
}
static int cmd_stream(const struct shell *sh, size_t argc, char **argv) {
  ARG_UNUSED(sh);
  ARG_UNUSED(argc);
  bool on = !strcmp(argv[1], "on");
  if (!on && strcmp(argv[1], "off"))
    return -EINVAL;
  k_mutex_lock(&data_lock, K_FOREVER);
  stream = on;
  k_mutex_unlock(&data_lock);
  return 0;
}
SHELL_STATIC_SUBCMD_SET_CREATE(
    thermal_cmds,
    SHELL_CMD_ARG(read, NULL, "Request acquisition", cmd_read, 1, 0),
    SHELL_CMD_ARG(status, NULL, "Sensor, source and last valid sample",
                  cmd_status, 1, 0),
    SHELL_CMD_ARG(threshold, NULL, "threshold <warning C> <critical C>",
                  cmd_threshold, 3, 0),
    SHELL_CMD_ARG(simulate, NULL,
                  "simulate <-40..125 C|off>; explicitly marked test mode",
                  cmd_simulate, 2, 0),
    SHELL_CMD_ARG(stream, NULL, "stream on|off", cmd_stream, 2, 0),
    SHELL_SUBCMD_SET_END);
SHELL_CMD_REGISTER(thermal, &thermal_cmds, "Onboard P3T1755 monitor", NULL);

int main(void) {
  int rc = rgb_init();
  check("RGB initialization", rc == 0);
  if (rc)
    return rc;
  check("P3T1755 device ready", device_is_ready(sensor));
  if (IS_ENABLED(CONFIG_APP_AUTOTEST)) {
    check("TEST classifier NORMAL 25C", classify(2500, 3500, 4500) == NORMAL);
    check("TEST classifier WARNING boundary 35C",
          classify(3500, 3500, 4500) == WARNING);
    check("TEST classifier CRITICAL boundary 45C",
          classify(4500, 3500, 4500) == CRITICAL);
    check("TEST classifier negative temperature",
          classify(-1000, 3500, 4500) == NORMAL);
  }
  report("BOOT", "P3T1755 via official I3C driver; actual sensor readings only "
                 "unless thermal simulate selected");
  k_sem_give(&acquire_ready);
  return 0;
}
