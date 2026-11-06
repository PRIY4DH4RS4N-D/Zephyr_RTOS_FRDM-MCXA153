#include "app.h"

#define DEBOUNCE_MS 30
enum state { IDLE, RUN, ALERT, CONFIGURATION, STATE_COUNT };
static const char *const state_names[] = {"IDLE", "RUN", "ALERT",
                                          "CONFIGURATION"};
static const unsigned colors[] = {0, 2, 1, 4};
struct button {
  struct gpio_dt_spec pin;
  struct gpio_callback callback;
  struct k_work_delayable debounce_work;
  unsigned index;
};
struct event {
  unsigned button;
  bool synthetic;
};
static struct button buttons[] = {
    {.pin = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios), .index = 0},
    {.pin = GPIO_DT_SPEC_GET(DT_ALIAS(sw1), gpios), .index = 1}};
K_MSGQ_DEFINE(events, sizeof(struct event), 8, 4);
K_MUTEX_DEFINE(state_lock);
K_SEM_DEFINE(test_done, 0, 1);
static enum state current = IDLE;
static atomic_t raw_count, accepted_count, debounced_count, overflow_count,
    io_errors, synthetic_count;

/* Pure transition rule is also used by predefined tests. SW2 cycles states;
 * SW3 acknowledges ALERT or toggles CONFIGURATION from any other state. */
static enum state transition(enum state s, unsigned button) {
  if (button == 0)
    return (s + 1) % STATE_COUNT;
  if (s == ALERT || s == CONFIGURATION)
    return IDLE;
  return CONFIGURATION;
}
static void debounce(struct k_work *work) {
  struct button *button = CONTAINER_OF(k_work_delayable_from_work(work),
                                       struct button, debounce_work);
  int active = gpio_pin_get_dt(&button->pin);
  if (active < 0) {
    atomic_inc(&io_errors);
    return;
  }
  if (!active) {
    atomic_inc(&debounced_count);
    return;
  }
  struct event event = {.button = button->index};
  if (k_msgq_put(&events, &event, K_NO_WAIT))
    atomic_inc(&overflow_count);
  else
    atomic_inc(&accepted_count);
}
static void interrupt(const struct device *port, struct gpio_callback *cb,
                      gpio_port_pins_t pins) {
  ARG_UNUSED(port);
  ARG_UNUSED(pins);
  struct button *button = CONTAINER_OF(cb, struct button, callback);
  atomic_inc(&raw_count);
  int rc = k_work_reschedule(&button->debounce_work, K_MSEC(DEBOUNCE_MS));
  if (rc == 1)
    atomic_inc(&debounced_count);
  else if (rc < 0)
    atomic_inc(&io_errors);
}
static void event_thread(void *a, void *b, void *c) {
  ARG_UNUSED(a);
  ARG_UNUSED(b);
  ARG_UNUSED(c);
  struct event event;
  for (;;) {
    k_msgq_get(&events, &event, K_FOREVER);
    k_mutex_lock(&state_lock, K_FOREVER);
    enum state before = current;
    current = transition(current, event.button);
    int rc = rgb_set(colors[current]);
    report(rc ? "FAIL" : "EVENT", "source=%s button=SW%u %s->%s led_rc=%d",
           event.synthetic ? "TEST" : "GPIO", event.button + 2,
           state_names[before], state_names[current], rc);
    k_mutex_unlock(&state_lock);
    if (event.synthetic) {
      atomic_inc(&synthetic_count);
      k_sem_give(&test_done);
    }
  }
}
K_THREAD_DEFINE(fsm_thread, 1280, event_thread, NULL, NULL, NULL, 4, 0, 0);
static int cmd_status(const struct shell *sh, size_t argc, char **argv) {
  ARG_UNUSED(sh);
  ARG_UNUSED(argc);
  ARG_UNUSED(argv);
  k_mutex_lock(&state_lock, K_FOREVER);
  report("STATUS",
         "mode=%s raw=%ld accepted=%ld debounced=%ld overflow=%ld "
         "io_errors=%ld synthetic=%ld",
         state_names[current], (long)atomic_get(&raw_count),
         (long)atomic_get(&accepted_count), (long)atomic_get(&debounced_count),
         (long)atomic_get(&overflow_count), (long)atomic_get(&io_errors),
         (long)atomic_get(&synthetic_count));
  k_mutex_unlock(&state_lock);
  return 0;
}
static int cmd_mode(const struct shell *sh, size_t argc, char **argv) {
  ARG_UNUSED(sh);
  ARG_UNUSED(argc);
  for (unsigned i = 0; i < STATE_COUNT; i++)
    if (!strcmp(argv[1], state_names[i])) {
      k_mutex_lock(&state_lock, K_FOREVER);
      current = i;
      int rc = rgb_set(colors[i]);
      report("MODE", "%s rc=%d", state_names[i], rc);
      k_mutex_unlock(&state_lock);
      return rc;
    }
  return -EINVAL;
}
SHELL_STATIC_SUBCMD_SET_CREATE(
    fsm_cmds,
    SHELL_CMD_ARG(status, NULL, "Show state and IRQ counters", cmd_status, 1,
                  0),
    SHELL_CMD_ARG(mode, NULL, "mode IDLE|RUN|ALERT|CONFIGURATION", cmd_mode, 2,
                  0),
    SHELL_SUBCMD_SET_END);
SHELL_CMD_REGISTER(fsm, &fsm_cmds, "Button state machine", NULL);
SHELL_CMD_ARG_REGISTER(status, NULL, "FSM status", cmd_status, 1, 0);
SHELL_CMD_ARG_REGISTER(mode, NULL, "mode IDLE|RUN|ALERT|CONFIGURATION",
                       cmd_mode, 2, 0);

static int init_buttons(void) {
  for (unsigned i = 0; i < ARRAY_SIZE(buttons); i++) {
    struct button *b = &buttons[i];
    if (!gpio_is_ready_dt(&b->pin))
      return -ENODEV;
    k_work_init_delayable(&b->debounce_work, debounce);
    int rc = gpio_pin_configure_dt(&b->pin, GPIO_INPUT);
    if (rc)
      return rc;
    gpio_init_callback(&b->callback, interrupt, BIT(b->pin.pin));
    rc = gpio_add_callback(b->pin.port, &b->callback);
    if (rc)
      return rc;
    rc = gpio_pin_interrupt_configure_dt(&b->pin, GPIO_INT_EDGE_TO_ACTIVE);
    if (rc)
      return rc;
  }
  return 0;
}
int main(void) {
  int rc = rgb_init();
  check("RGB initialization", rc == 0);
  if (rc)
    return rc;
  if (IS_ENABLED(CONFIG_APP_AUTOTEST)) {
    check("SW2 IDLE->RUN", transition(IDLE, 0) == RUN);
    check("SW2 RUN->ALERT", transition(RUN, 0) == ALERT);
    check("SW3 acknowledges ALERT", transition(ALERT, 1) == IDLE);
    struct event test = {.button = 0, .synthetic = true};
    check("Synthetic deferred event queued",
          k_msgq_put(&events, &test, K_NO_WAIT) == 0);
    check("Deferred FSM processed test",
          k_sem_take(&test_done, K_SECONDS(1)) == 0);
    k_mutex_lock(&state_lock, K_FOREVER);
    current = IDLE;
    rgb_set(0);
    k_mutex_unlock(&state_lock);
  }
  rc = init_buttons();
  check("SW2/SW3 interrupt setup", rc == 0);
  if (rc) {
    report("FAIL", "button initialization rc=%d", rc);
    return rc;
  }
  report("BOOT", "SW2=GPIO3.29 SW3=GPIO1.7 debounce=30ms; physical IRQ "
                 "verification PENDING");
  return 0;
}
