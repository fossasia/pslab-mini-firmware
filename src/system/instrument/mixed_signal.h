#ifndef MIXED_SIGNAL_H
#define MIXED_SIGNAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "system/logic_analyser.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    MIXED_SIGNAL_MAX_SAMPLES = 4096,
};

typedef enum {
    MIXED_SIGNAL_STATUS_IDLE = 0,
    MIXED_SIGNAL_STATUS_READY = 1,
    MIXED_SIGNAL_STATUS_BUSY = 2,
} MixedSignalStatus;

typedef struct {
    uint32_t sample_rate_hz;
    uint32_t sample_count;
    uint32_t digital_pin_base;
    uint32_t digital_pin_count;
    uint32_t digital_word_count;
    uint32_t digital_bits_per_word;
    uint32_t analog_channel;
    uint32_t analog_gpio;
    uint32_t trigger_pin;
    bool trigger_level;
    LogicAnalyserTriggerMode trigger_mode;
    int32_t digital_start_offset_ns;
    int32_t analog_start_offset_ns;
} MixedSignalCaptureInfo;

void mixed_signal_reset_state(void);

bool mixed_signal_set_digital_pin_base(uint32_t value);
bool mixed_signal_set_digital_pin_count(uint32_t value);
bool mixed_signal_set_analog_channel(uint32_t value);
bool mixed_signal_set_sample_rate(uint32_t value);
bool mixed_signal_set_samples(uint32_t value);
bool mixed_signal_set_trigger_pin(uint32_t value);
void mixed_signal_set_trigger_level(bool value);
bool mixed_signal_set_trigger_mode_edge(bool edge_mode);

uint32_t mixed_signal_get_digital_pin_base(void);
uint32_t mixed_signal_get_digital_pin_count(void);
uint32_t mixed_signal_get_analog_channel(void);
uint32_t mixed_signal_get_sample_rate(void);
uint32_t mixed_signal_get_samples(void);
uint32_t mixed_signal_get_trigger_pin(void);
bool mixed_signal_get_trigger_level(void);
bool mixed_signal_get_trigger_mode_edge(void);
MixedSignalCaptureInfo const *mixed_signal_get_last_info(void);

bool mixed_signal_initiate(void);
bool mixed_signal_fetch_digital(uint8_t const **data, size_t *len);
bool mixed_signal_fetch_analog(uint8_t const **data, size_t *len);
MixedSignalStatus mixed_signal_status(void);

#ifdef __cplusplus
}
#endif

#endif
