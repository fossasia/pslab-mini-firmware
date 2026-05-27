#ifndef TEST_SIGNAL_H
#define TEST_SIGNAL_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    TEST_SIGNAL_DEFAULT_PIN = 15,
    TEST_SIGNAL_DEFAULT_FREQUENCY_HZ = 1000,
};

void test_signal_init(void);
bool test_signal_start(uint32_t gpio, uint32_t frequency_hz);
void test_signal_stop(void);
bool test_signal_is_enabled(void);
uint32_t test_signal_get_pin(void);
uint32_t test_signal_get_frequency_hz(void);

#ifdef __cplusplus
}
#endif

#endif
