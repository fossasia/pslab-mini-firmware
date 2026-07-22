#include "platform/test_signal.h"

#include <stdbool.h>

#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"

static bool enabled;
static uint32_t output_pin = TEST_SIGNAL_DEFAULT_PIN;
static uint32_t frequency_hz = TEST_SIGNAL_DEFAULT_FREQUENCY_HZ;
static bool analog_enabled;
static uint32_t analog_output_pin = TEST_SIGNAL_ANALOG_DEFAULT_PIN;
static uint32_t analog_frequency_hz = TEST_SIGNAL_ANALOG_DEFAULT_FREQUENCY_HZ;
static uint32_t analog_duty_permille = TEST_SIGNAL_ANALOG_DEFAULT_DUTY_PERMILLE;

static bool pwm_slice_conflicts(uint32_t a, uint32_t b)
{
    return pwm_gpio_to_slice_num((uint)a) == pwm_gpio_to_slice_num((uint)b);
}

void test_signal_init(void)
{
    test_signal_stop();
    test_signal_analog_stop();
}

bool test_signal_start(uint32_t gpio, uint32_t requested_frequency_hz)
{
    if (gpio > 29 || requested_frequency_hz == 0) {
        return false;
    }
    if (analog_enabled && pwm_slice_conflicts(gpio, analog_output_pin)) {
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

bool test_signal_analog_start(
    uint32_t gpio,
    uint32_t requested_frequency_hz,
    uint32_t requested_duty_permille
)
{
    if (gpio > 29 || requested_frequency_hz == 0 ||
        requested_duty_permille > 1000) {
        return false;
    }
    if (enabled && pwm_slice_conflicts(gpio, output_pin)) {
        return false;
    }

    test_signal_analog_stop();

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

    uint32_t level = ((wrap + 1) * requested_duty_permille) / 1000;
    if (level > wrap + 1) {
        level = wrap + 1;
    }

    gpio_set_function((uint)gpio, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num((uint)gpio);
    uint channel = pwm_gpio_to_channel((uint)gpio);

    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, divider);
    pwm_config_set_wrap(&config, (uint16_t)wrap);
    pwm_init(slice, &config, false);
    pwm_set_chan_level(slice, channel, (uint16_t)level);
    pwm_set_enabled(slice, true);

    analog_output_pin = gpio;
    analog_frequency_hz = requested_frequency_hz;
    analog_duty_permille = requested_duty_permille;
    analog_enabled = true;
    return true;
}

void test_signal_analog_stop(void)
{
    if (analog_enabled) {
        uint slice = pwm_gpio_to_slice_num((uint)analog_output_pin);
        pwm_set_enabled(slice, false);
        gpio_init(analog_output_pin);
        gpio_set_dir(analog_output_pin, GPIO_OUT);
        gpio_put(analog_output_pin, 0);
    }

    analog_enabled = false;
}

bool test_signal_analog_is_enabled(void)
{
    return analog_enabled;
}

uint32_t test_signal_analog_get_pin(void)
{
    return analog_output_pin;
}

uint32_t test_signal_analog_get_frequency_hz(void)
{
    return analog_frequency_hz;
}

uint32_t test_signal_analog_get_duty_permille(void)
{
    return analog_duty_permille;
}
