#include "system/adc_capture.h"

#include "hardware/adc.h"
#include "hardware/dma.h"

enum {
    ADC_CLOCK_HZ = 48000000,
    ADC_CONVERSION_CYCLES = 96,
};

static AdcCaptureConfig active_config;
static int dma_chan = -1;
static bool initialized;
static bool busy;

uint32_t adc_capture_channel_to_gpio(uint32_t channel)
{
    return 26u + channel;
}

static bool config_is_valid(AdcCaptureConfig const *config)
{
    return config && config->channel <= ADC_CAPTURE_MAX_CHANNEL &&
           config->sample_rate_hz >= ADC_CAPTURE_MIN_SAMPLE_RATE_HZ &&
           config->sample_rate_hz <= ADC_CAPTURE_MAX_SAMPLE_RATE_HZ;
}

static float clock_divider_for_rate(uint32_t sample_rate_hz)
{
    if (sample_rate_hz >= ADC_CAPTURE_MAX_SAMPLE_RATE_HZ) {
        return 0.0f;
    }

    float divider = (float)ADC_CLOCK_HZ / (float)sample_rate_hz - 1.0f;
    if (divider < (float)ADC_CONVERSION_CYCLES) {
        return 0.0f;
    }
    return divider;
}

bool adc_capture_init(AdcCaptureConfig const *config)
{
    if (!config_is_valid(config)) {
        return false;
    }

    if (initialized) {
        return adc_capture_configure(config);
    }

    dma_chan = dma_claim_unused_channel(false);
    if (dma_chan < 0) {
        return false;
    }

    adc_init();
    initialized = true;
    return adc_capture_configure(config);
}

void adc_capture_deinit(void)
{
    if (!initialized) {
        return;
    }

    adc_run(false);
    adc_fifo_drain();
    if (dma_chan >= 0) {
        dma_channel_abort((uint)dma_chan);
        dma_channel_unclaim((uint)dma_chan);
    }
    dma_chan = -1;
    initialized = false;
    busy = false;
}

bool adc_capture_configure(AdcCaptureConfig const *config)
{
    if (!initialized || busy || !config_is_valid(config)) {
        return false;
    }

    active_config = *config;
    adc_gpio_init(adc_capture_channel_to_gpio(active_config.channel));
    adc_select_input(active_config.channel);
    adc_fifo_setup(
        true,
        true,
        1,
        false,
        false
    );
    adc_set_clkdiv(clock_divider_for_rate(active_config.sample_rate_hz));
    return true;
}

bool adc_capture_run(
    uint16_t *buffer,
    uint32_t sample_count,
    AdcCaptureInfo *info
)
{
    if (!initialized || busy || !buffer || sample_count == 0) {
        return false;
    }

    adc_run(false);
    adc_fifo_drain();
    adc_select_input(active_config.channel);
    adc_set_clkdiv(clock_divider_for_rate(active_config.sample_rate_hz));

    dma_channel_config dma_config =
        dma_channel_get_default_config((uint)dma_chan);
    channel_config_set_transfer_data_size(&dma_config, DMA_SIZE_16);
    channel_config_set_read_increment(&dma_config, false);
    channel_config_set_write_increment(&dma_config, true);
    channel_config_set_dreq(&dma_config, DREQ_ADC);

    busy = true;
    dma_channel_configure(
        (uint)dma_chan,
        &dma_config,
        buffer,
        &adc_hw->fifo,
        sample_count,
        true
    );

    adc_run(true);
    dma_channel_wait_for_finish_blocking((uint)dma_chan);
    adc_run(false);
    adc_fifo_drain();
    busy = false;

    if (info) {
        *info = (AdcCaptureInfo){
            .channel = active_config.channel,
            .gpio = adc_capture_channel_to_gpio(active_config.channel),
            .sample_rate_hz = active_config.sample_rate_hz,
            .sample_count = sample_count,
        };
    }

    return true;
}

bool adc_capture_read_once(uint16_t *sample)
{
    if (!initialized || busy || !sample) {
        return false;
    }

    adc_select_input(active_config.channel);
    *sample = adc_read();
    return true;
}

bool adc_capture_is_busy(void) { return busy; }
