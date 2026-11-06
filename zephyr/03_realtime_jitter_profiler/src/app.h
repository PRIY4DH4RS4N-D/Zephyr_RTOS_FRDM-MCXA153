#pragma once
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
void report(const char *tag, const char *fmt, ...) __printf_like(2, 3);
int rgb_init(void);
int rgb_set(unsigned mask);
int number(const char *s, long lo, long hi, long *out);
void check(const char *name, bool ok);
