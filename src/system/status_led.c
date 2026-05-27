#include "system/status_led.h"

#include <stdbool.h>

#include "pico/stdlib.h"

enum {
    BLINK_ON_MS = 80,
    BLINK_OFF_MS = 80,
};

static bool initialized;

void status_led_init(void)
{
#ifdef PICO_DEFAULT_LED_PIN
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, 0);
#endif
    initialized = true;
}

void status_led_set(bool on)
{
    if (!initialized) {
        return;
    }

#ifdef PICO_DEFAULT_LED_PIN
    gpio_put(PICO_DEFAULT_LED_PIN, on ? 1 : 0);
#else
    (void)on;
#endif
}

void status_led_blink(unsigned int count)
{
    if (!initialized) {
        return;
    }

    for (unsigned int i = 0; i < count; ++i) {
        status_led_set(true);
        sleep_ms(BLINK_ON_MS);
        status_led_set(false);
        sleep_ms(BLINK_OFF_MS);
    }
}

void status_led_command_received(void)
{
    status_led_blink(1);
}

void status_led_capture_started(void)
{
    status_led_blink(2);
    status_led_set(true);
}

void status_led_capture_finished(void)
{
    status_led_blink(2);
    status_led_set(false);
}
