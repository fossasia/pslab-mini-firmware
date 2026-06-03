#ifndef ADC_CAPTURE_H
#define ADC_CAPTURE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    ADC_CAPTURE_MAX_CHANNEL = 3,
    ADC_CAPTURE_MAX_SAMPLE_RATE_HZ = 500000,
    ADC_CAPTURE_MIN_SAMPLE_RATE_HZ = 1,
};

typedef struct {
    uint32_t channel;
    uint32_t sample_rate_hz;
} AdcCaptureConfig;

typedef struct {
    uint32_t channel;
    uint32_t gpio;
    uint32_t sample_rate_hz;
    uint32_t sample_count;
} AdcCaptureInfo;

bool adc_capture_init(AdcCaptureConfig const *config);
void adc_capture_deinit(void);
bool adc_capture_configure(AdcCaptureConfig const *config);
bool adc_capture_run(
    uint16_t *buffer,
    uint32_t sample_count,
    AdcCaptureInfo *info
);
bool adc_capture_read_once(uint16_t *sample);
bool adc_capture_is_busy(void);
uint32_t adc_capture_channel_to_gpio(uint32_t channel);

#ifdef __cplusplus
}
#endif

#endif
