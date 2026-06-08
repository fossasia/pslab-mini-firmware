/**
 * @file dso.h
 * @brief Digital Storage Oscilloscope interface for PSLab Pico firmware
 *
 * This header preserves the DSO instrument boundary from the STM32 firmware
 * while adapting the implementation to the RP2350 internal ADC. The current
 * Pico DSO supports one ADC channel at a time and exposes trigger/capture
 * helpers used by the SCPI application layer.
 *
 * @author PSLab Team
 * @date 2025-09-29
 */

#ifndef PSLAB_DSO_H
#define PSLAB_DSO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // PSLAB_DSO_H

enum {
    DSO_MAX_SAMPLES = 4096,
};

typedef struct {
    uint32_t channel;
    uint32_t gpio;
    uint32_t sample_rate_hz;
    uint32_t sample_count;
} DsoCaptureInfo;

typedef enum {
    DSO_TRIGGER_OFF,
    DSO_TRIGGER_LEVEL,
    DSO_TRIGGER_EDGE,
} DsoTriggerMode;

typedef enum {
    DSO_TRIGGER_RISING,
    DSO_TRIGGER_FALLING,
} DsoTriggerSlope;

void dso_reset_state(void);

bool dso_set_channel(uint32_t value);
bool dso_set_sample_rate(uint32_t value);
bool dso_set_samples(uint32_t value);
bool dso_set_trigger_level(uint32_t value);
bool dso_set_trigger_mode(DsoTriggerMode mode);
bool dso_set_trigger_slope(DsoTriggerSlope slope);

uint32_t dso_get_channel(void);
uint32_t dso_get_gpio(void);
uint32_t dso_get_sample_rate(void);
uint32_t dso_get_samples(void);
uint32_t dso_get_trigger_level(void);
DsoTriggerMode dso_get_trigger_mode(void);
DsoTriggerSlope dso_get_trigger_slope(void);

bool dso_initiate(void);
bool dso_fetch(uint8_t const **data, size_t *len);
uint32_t dso_status(void);

bool dso_stream_start(void);
void dso_stream_stop(void);
bool dso_stream_is_enabled(void);
uint32_t dso_stream_get_sequence(void);
uint32_t dso_stream_get_overruns(void);
bool dso_stream_next_frame(uint8_t const **data, size_t *len, uint32_t *sequence);

#ifdef __cplusplus
}
#endif

#endif
