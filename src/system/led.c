/**
 * @file led.c
 * @brief LED system implementation - simple wrapper around the led_ll interface
 *
 * This module implements the LED system interface by wrapping calls to the
 * low-level LED hardware abstraction layer (led_ll). It provides a clean
 * separation between the application layer and hardware-specific
 * implementation.
 */

#include "platform/led_ll.h"
#include "util/error.h"

#include "led.h"

void LED_init(void) { LED_LL_init(); }

void LED_toggle(void) { LED_LL_toggle(); }
