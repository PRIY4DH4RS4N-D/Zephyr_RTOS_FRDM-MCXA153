#include "app.h"
#include <stdarg.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(portfolio, LOG_LEVEL_INF);
static const struct gpio_dt_spec leds[] = {
    GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios)};

K_MUTEX_DEFINE(led_lock);
void report(const char *tag, const char *fmt, ...) {
  char b[320];
  va_list ap;
  va_start(ap, fmt);
  vsnprintk(b, sizeof(b), fmt, ap);
  va_end(ap);
  LOG_INF("[%lld ms] [%s] %s", k_uptime_get(), tag, b);
}
int rgb_init(void) {
  for (size_t i = 0; i < ARRAY_SIZE(leds); i++) {
    if (!gpio_is_ready_dt(&leds[i]))
      return -ENODEV;
    int r = gpio_pin_configure_dt(&leds[i], GPIO_OUTPUT_INACTIVE);
    if (r)
      return r;
  }
  return 0;
}
int rgb_set(unsigned mask) {
  int ret = 0;
  k_mutex_lock(&led_lock, K_FOREVER);
  for (unsigned i = 0; i < 3; i++) {
    int r = gpio_pin_set_dt(&leds[i], !!(mask & BIT(i)));
    if (r)
      ret = r;
  }
  k_mutex_unlock(&led_lock);
  return ret;
}
int number(const char *s, long lo, long hi, long *out) {
  char *e;
  errno = 0;
  long n = strtol(s, &e, 10);
  if (errno || e == s || *e || n < lo || n > hi)
    return -EINVAL;
  *out = n;
  return 0;
}
void check(const char *name, bool ok) {
  report(ok ? "PASS" : "FAIL", "%s", name);
}

/* All threads are statically allocated; unlocked traversal permits logging. */
static void show_thread(const struct k_thread *thread, void *user) {
  ARG_UNUSED(user);
  size_t unused = 0;
  int rc = k_thread_stack_space_get(thread, &unused);
  const char *name = k_thread_name_get((k_tid_t)thread);
  report("STACK", "thread=%s priority=%d reserved=%u unused_watermark=%u rc=%d",
         name ? name : "unnamed", k_thread_priority_get((k_tid_t)thread),
         (unsigned)thread->stack_info.size, (unsigned)unused, rc);
}
static int resources(const struct shell *sh, size_t argc, char **argv) {
  ARG_UNUSED(sh); ARG_UNUSED(argc); ARG_UNUSED(argv);
  report("RESOURCE", "heap_pool=%u bytes; stack unused values are runtime watermarks",
         CONFIG_HEAP_MEM_POOL_SIZE);
  k_thread_foreach_unlocked(show_thread, NULL);
  return 0;
}
SHELL_CMD_ARG_REGISTER(resources, NULL, "Runtime thread stack watermarks", resources, 1, 0);
