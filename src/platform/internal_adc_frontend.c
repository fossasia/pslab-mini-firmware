#include "platform/internal_adc_frontend.h"

#include "platform/adc_capture.h"
#include "system/instrument/adc_frontend.h"

#include <stdbool.h>
#include <stdint.h>

enum {
    INTERNAL_ADC_MAX_SAMPLE_COUNT = 4096,
};

static AdcCaptureConfig to_capture_config(AdcFrontendConfig const *config)
{
    return (AdcCaptureConfig){
        .channel = config->channel,
        .sample_rate_hz = config->sample_rate_hz,
    };
}

static void to_frontend_info(
    AdcCaptureInfo const *capture_info,
    AdcFrontendCaptureInfo *frontend_info
)
{
    if (!capture_info || !frontend_info) {
        return;
    }

    *frontend_info = (AdcFrontendCaptureInfo){
        .backend = ADC_FRONTEND_BACKEND_INTERNAL,
        .channel = capture_info->channel,
        .gpio = capture_info->gpio,
        .sample_rate_hz = capture_info->sample_rate_hz,
        .sample_count = capture_info->sample_count,
        .sample_width_bits = 12,
    };
}

static bool internal_init(AdcFrontendConfig const *config)
{
    if (!config || config->backend != ADC_FRONTEND_BACKEND_INTERNAL) {
        return false;
    }

    AdcCaptureConfig capture_config = to_capture_config(config);
    return adc_capture_init(&capture_config);
}

static void internal_deinit(void)
{
    adc_capture_deinit();
}

static bool internal_configure(AdcFrontendConfig const *config)
{
    if (!config || config->backend != ADC_FRONTEND_BACKEND_INTERNAL) {
        return false;
    }

    AdcCaptureConfig capture_config = to_capture_config(config);
    return adc_capture_configure(&capture_config);
}

static bool internal_run(
    uint16_t *buffer,
    uint32_t sample_count,
    AdcFrontendCaptureInfo *info
)
{
    AdcCaptureInfo capture_info;
    if (!adc_capture_run(buffer, sample_count, &capture_info)) {
        return false;
    }

    to_frontend_info(&capture_info, info);
    return true;
}

static bool internal_arm(
    uint16_t *buffer,
    uint32_t sample_count,
    AdcFrontendCaptureInfo *info
)
{
    AdcCaptureInfo capture_info;
    if (!adc_capture_arm(buffer, sample_count, &capture_info)) {
        return false;
    }

    to_frontend_info(&capture_info, info);
    return true;
}

static AdcFrontendDriver const internal_driver = {
    .backend = ADC_FRONTEND_BACKEND_INTERNAL,
    .name = "internal",
    .capabilities = {
        .backend = ADC_FRONTEND_BACKEND_INTERNAL,
        .name = "internal",
        .max_channel = ADC_CAPTURE_MAX_CHANNEL,
        .min_sample_rate_hz = ADC_CAPTURE_MIN_SAMPLE_RATE_HZ,
        .max_sample_rate_hz = ADC_CAPTURE_MAX_SAMPLE_RATE_HZ,
        .max_sample_count = INTERNAL_ADC_MAX_SAMPLE_COUNT,
        .sample_width_bits = 12,
    },
    .init = internal_init,
    .deinit = internal_deinit,
    .configure = internal_configure,
    .run = internal_run,
    .arm = internal_arm,
    .start = adc_capture_start,
    .wait = adc_capture_wait,
    .abort = adc_capture_abort,
    .read_once = adc_capture_read_once,
    .is_busy = adc_capture_is_busy,
    .channel_to_gpio = adc_capture_channel_to_gpio,
};

void internal_adc_frontend_register(void)
{
    (void)adc_frontend_register_driver(&internal_driver);
}
