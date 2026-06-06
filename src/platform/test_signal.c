#include "platform/test_signal.h"

#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"

static bool enabled;
static uint32_t output_pin = TEST_SIGNAL_DEFAULT_PIN;
static uint32_t frequency_hz = TEST_SIGNAL_DEFAULT_FREQUENCY_HZ;

void test_signal_init(void)
{
    test_signal_stop();
}

bool test_signal_start(uint32_t gpio, uint32_t requested_frequency_hz)
{
    if (gpio > 29 || requested_frequency_hz == 0) {
        return false;
    }

    test_signal_stop();

    uint32_t clock_hz = clock_get_hz(clk_sys);
    uint32_t period_counts = clock_hz / requested_frequency_hz;
    if (period_counts < 2) {
        return false;
    }

    float divider = 1.0f;
    uint32_t wrap = period_counts - 1;
    if (wrap > 65535) {
        wrap = 65535;
        divider = (float)clock_hz /
                  ((float)requested_frequency_hz * (float)(wrap + 1));
    }

    if (divider < 1.0f || divider > 255.0f) {
        return false;
    }

    gpio_set_function((uint)gpio, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num((uint)gpio);
    uint channel = pwm_gpio_to_channel((uint)gpio);

    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, divider);
    pwm_config_set_wrap(&config, (uint16_t)wrap);
    pwm_init(slice, &config, false);
    pwm_set_chan_level(slice, channel, (uint16_t)((wrap + 1) / 2));
    pwm_set_enabled(slice, true);

    output_pin = gpio;
    frequency_hz = requested_frequency_hz;
    enabled = true;
    return true;
}

void test_signal_stop(void)
{
    if (enabled) {
        uint slice = pwm_gpio_to_slice_num((uint)output_pin);
        pwm_set_enabled(slice, false);
        gpio_init(output_pin);
        gpio_set_dir(output_pin, GPIO_OUT);
        gpio_put(output_pin, 0);
    }

    enabled = false;
}

bool test_signal_is_enabled(void)
{
    return enabled;
}

uint32_t test_signal_get_pin(void)
{
    return output_pin;
}

uint32_t test_signal_get_frequency_hz(void)
{
    return frequency_hz;
}
