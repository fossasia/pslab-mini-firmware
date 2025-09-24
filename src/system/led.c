/**
 * @file led.c
 * @brief LED system implementation - simple wrapper around the led_ll interface
 *
 * This module implements the LED system interface by wrapping calls to the
 * low-level LED hardware abstraction layer (led_ll).
 */

#include "platform/led_ll.h"
#include "util/logging.h"

#include "led.h"

static LED_LL_ID const g_LED_MAPPING[LED_COUNT] = {
    [LED_GREEN] = LED_LL_GREEN,
    [LED_YELLOW] = LED_LL_YELLOW,
    [LED_RED] = LED_LL_RED,
};

static char const *const g_LED_INVALID_WARN_MSG =
    "Attempted to access invalid LED ID: %d";

void LED_init(void)
{
    // Silence unused variable warning when log level is <= WARN
    (void)g_LED_INVALID_WARN_MSG;
    LED_LL_init();
}

void LED_on(LED_ID led_id)
{
    if (led_id < LED_COUNT) {
        LED_LL_on(g_LED_MAPPING[led_id]);
        return;
    }
    LOG_WARN(g_LED_INVALID_WARN_MSG, led_id);
}

void LED_off(LED_ID led_id)
{
    if (led_id < LED_COUNT) {
        LED_LL_off(g_LED_MAPPING[led_id]);
        return;
    }
    LOG_WARN(g_LED_INVALID_WARN_MSG, led_id);
}

void LED_toggle(LED_ID led_id)
{
    if (led_id < LED_COUNT) {
        LED_LL_toggle(g_LED_MAPPING[led_id]);
        return;
    }
    LOG_WARN(g_LED_INVALID_WARN_MSG, led_id);
}
