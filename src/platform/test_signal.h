#ifndef TEST_SIGNAL_H
#define TEST_SIGNAL_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    TEST_SIGNAL_DEFAULT_PIN = 15,
    TEST_SIGNAL_ANALOG_DEFAULT_PIN = 13,
    TEST_SIGNAL_DEFAULT_FREQUENCY_HZ = 1000,
    TEST_SIGNAL_ANALOG_DEFAULT_FREQUENCY_HZ = 1000,
    TEST_SIGNAL_ANALOG_DEFAULT_DUTY_PERMILLE = 500,
};

void test_signal_init(void);
bool test_signal_start(uint32_t gpio, uint32_t frequency_hz);
void test_signal_stop(void);
bool test_signal_is_enabled(void);
uint32_t test_signal_get_pin(void);
uint32_t test_signal_get_frequency_hz(void);
bool test_signal_analog_start(
    uint32_t gpio,
    uint32_t frequency_hz,
    uint32_t duty_permille
);
void test_signal_analog_stop(void);
bool test_signal_analog_is_enabled(void);
uint32_t test_signal_analog_get_pin(void);
uint32_t test_signal_analog_get_frequency_hz(void);
uint32_t test_signal_analog_get_duty_permille(void);

#ifdef __cplusplus
}
#endif

#endif
