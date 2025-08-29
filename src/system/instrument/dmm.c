/**
 * @file dmm.c
 * @brief Digital Multimeter implementation for PSLab firmware
 *
 * This file implements a simple digital multimeter using the ADC_LL API
 * in single sample mode. It provides voltage measurement functionality with
 * proper error handling and logging.
 *
 * @author PSLab Team
 * @date 2025-07-18
 */

#include "dmm.h"
#include "adc_ll.h"
#include "error.h"
#include "fixed_point.h"
#include "logging.h"
#include "si_prefix.h"
#include "tim_ll.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

/**
 * @brief DMM handle structure
 */
struct DMM_Handle {
    DMM_Config config;
    uint16_t adc_value; // Single value buffer for single sample mode
    bool volatile conversion_complete;
    bool initialized;
};

// Static instance for callback context
static DMM_Handle *g_dmm_handle = NULL;

/**
 * @brief Convert DMM channel to ADC_LL channel
 */
static ADC_LL_Channel dmm_channel_to_adc_ll(DMM_Channel channel)
{
    // Explicit mapping to avoid linter warnings about enum casting
    switch (channel) {
    case DMM_CHANNEL_0:
        return ADC_LL_CHANNEL_0;
    case DMM_CHANNEL_1:
        return ADC_LL_CHANNEL_1;
    case DMM_CHANNEL_2:
        return ADC_LL_CHANNEL_2;
    case DMM_CHANNEL_3:
        return ADC_LL_CHANNEL_3;
    case DMM_CHANNEL_4:
        return ADC_LL_CHANNEL_4;
    case DMM_CHANNEL_5:
        return ADC_LL_CHANNEL_5;
    case DMM_CHANNEL_6:
        return ADC_LL_CHANNEL_6;
    case DMM_CHANNEL_7:
        return ADC_LL_CHANNEL_7;
    case DMM_CHANNEL_8:
        return ADC_LL_CHANNEL_8;
    case DMM_CHANNEL_9:
        return ADC_LL_CHANNEL_9;
    case DMM_CHANNEL_10:
        return ADC_LL_CHANNEL_10;
    case DMM_CHANNEL_11:
        return ADC_LL_CHANNEL_11;
    case DMM_CHANNEL_12:
        return ADC_LL_CHANNEL_12;
    case DMM_CHANNEL_13:
        return ADC_LL_CHANNEL_13;
    case DMM_CHANNEL_14:
        return ADC_LL_CHANNEL_14;
    case DMM_CHANNEL_15:
        return ADC_LL_CHANNEL_15;
    default:
        // This should never happen due to validation, but satisfies linter
        return ADC_LL_CHANNEL_0;
    }
}

/**
 * @brief ADC completion callback.
 *
 * Called when an ADC conversion is complete.
 */
// NOLINTNEXTLINE(readability-non-const-parameter)
void dmm_adc_complete_callback(uint16_t *buffer, uint32_t total_samples)
{
    (void)buffer;
    (void)total_samples;

    if (g_dmm_handle != NULL) {
        g_dmm_handle->conversion_complete = true;
        LOG_DEBUG(
            "DMM: ADC conversion complete, value = %u", g_dmm_handle->adc_value
        );
    }
}

/**
 * @brief Allocate and initialize DMM handle
 */
static DMM_Handle *dmm_create_handle(DMM_Config const *config)
{
    // Check if already initialized
    if (g_dmm_handle != NULL) {
        LOG_ERROR("DMM: Already initialized");
        THROW(ERROR_RESOURCE_BUSY);
    }

    // Allocate handle
    DMM_Handle *handle = (DMM_Handle *)malloc(sizeof(DMM_Handle));
    if (handle == NULL) {
        LOG_ERROR("DMM: Failed to allocate memory for handle");
        THROW(ERROR_OUT_OF_MEMORY);
    }

    // Initialize handle
    handle->config = *config;
    handle->adc_value = 0;
    handle->conversion_complete = false;
    handle->initialized = true;
    g_dmm_handle = handle;

    LOG_INFO(
        "DMM: Initializing with channel %d, oversampling ratio %u",
        config->channel,
        config->oversampling_ratio
    );

    return handle;
}

/**
 * @brief Create ADC configuration for DMM
 */
static ADC_LL_Config dmm_create_adc_config(DMM_Handle *handle)
{
    ADC_LL_Config adc_config = {
        .channels = { dmm_channel_to_adc_ll(handle->config.channel) },
        .channel_count = 1,
        .mode = ADC_LL_MODE_SINGLE,
        .trigger_source = ADC_TRIGGER_TIMER6,
        .output_buffer = &handle->adc_value,
        .buffer_size = 1, // Single sample
        .oversampling_ratio = handle->config.oversampling_ratio
    };
    return adc_config;
}

/**
 * @brief Initialize ADC for DMM operation
 */
static void dmm_init_adc(DMM_Handle *handle)
{
    // Set up ADC callback
    ADC_LL_set_complete_callback(dmm_adc_complete_callback);

    // Initialize ADC with the configured channel
    ADC_LL_Config adc_config = dmm_create_adc_config(handle);

    Error error = ERROR_NONE;
    TRY { ADC_LL_init(&adc_config); }
    CATCH(error)
    {
        LOG_ERROR("DMM: ADC initialization failed with error %d", error);
        g_dmm_handle = NULL;
        free(handle);
        THROW(error);
    }
}

/**
 * @brief Initialize timer for ADC triggering
 */
static void dmm_init_timer(DMM_Handle *handle)
{
    Error error = ERROR_NONE;
    TRY { TIM_LL_init(TIM_NUM_0, ADC_LL_get_sample_rate()); }
    CATCH(error)
    {
        LOG_ERROR("DMM: Timer initialization failed with error %d", error);
        ADC_LL_deinit();
        g_dmm_handle = NULL;
        free(handle);
        THROW(error);
    }
    LOG_DEBUG(
        "DMM: Timer initialized with frequency %u Hz", ADC_LL_get_sample_rate()
    );
}

/**
 * @brief Start ADC and timer for continuous operation
 */
static void dmm_start_conversion(DMM_Handle *handle)
{
    Error error = ERROR_NONE;
    TRY
    {
        TIM_LL_start(TIM_NUM_0); // Start timer for ADC triggering
        ADC_LL_start();
    }
    CATCH(error)
    {
        LOG_ERROR(
            "DMM: Failed to start initial conversion with error %d", error
        );
        TIM_LL_stop(TIM_NUM_0);
        ADC_LL_deinit();
        g_dmm_handle = NULL;
        free(handle);
        THROW(error);
    }
}

/**
 * @brief Validate DMM configuration
 */
static bool dmm_validate_config(DMM_Config const *config)
{
    if (config == NULL) {
        LOG_ERROR("DMM: Configuration is NULL");
        return false;
    }

    // Validate channel
    if (config->channel < DMM_CHANNEL_0 || config->channel > DMM_CHANNEL_15) {
        LOG_ERROR("DMM: Invalid DMM channel: %d", config->channel);
        return false;
    }

    // Validate oversampling ratio (must be power of 2, 1-256)
    uint32_t ratio = config->oversampling_ratio;
    if (ratio == 0 || ratio > 256 || (ratio & (ratio - 1)) != 0) {
        LOG_ERROR("DMM: Invalid oversampling ratio: %u", ratio);
        return false;
    }

    return true;
}

DMM_Handle *DMM_init(DMM_Config const *config)
{
    LOG_FUNCTION_ENTRY();

    if (!dmm_validate_config(config)) {
        LOG_ERROR("DMM: Invalid configuration");
        THROW(ERROR_INVALID_ARGUMENT);
    }

    DMM_Handle *handle = dmm_create_handle(config);
    dmm_init_adc(handle);
    dmm_init_timer(handle);
    dmm_start_conversion(handle);

    LOG_INFO("DMM: Initialized successfully and first conversion started");
    LOG_FUNCTION_EXIT();
    return handle;
}

void DMM_deinit(DMM_Handle *handle)
{
    LOG_FUNCTION_ENTRY();

    if (handle == NULL) {
        LOG_WARN("DMM: Attempt to deinitialize NULL handle");
        return;
    }

    if (!handle->initialized) {
        LOG_WARN("DMM: Handle not initialized");
        return;
    }

    LOG_INFO("DMM: Deinitializing");

    // Stop ADC and timer
    ADC_LL_stop();
    TIM_LL_stop(TIM_NUM_0);

    // Deinitialize ADC
    ADC_LL_deinit();

    // Clear global handle
    g_dmm_handle = NULL;

    // Mark as deinitialized and free memory
    handle->initialized = false;
    free(handle);

    LOG_INFO("DMM: Deinitialized successfully");
    LOG_FUNCTION_EXIT();
}

bool DMM_read_voltage(DMM_Handle *handle, FIXED_Q1616 *voltage_out)
{
    LOG_FUNCTION_ENTRY();

    if (handle == NULL || voltage_out == NULL) {
        LOG_ERROR(
            "DMM: Invalid arguments (handle=%p, voltage_out=%p)",
            handle,
            voltage_out
        );
        THROW(ERROR_INVALID_ARGUMENT);
    }

    if (!handle->initialized) {
        LOG_ERROR("DMM: Handle not initialized");
        THROW(ERROR_DEVICE_NOT_READY);
    }

    // Check if conversion is complete
    bool conversion_ready = handle->conversion_complete;

    if (conversion_ready) {
        // Get reference voltage from ADC driver
        uint32_t ref_voltage_mv = ADC_LL_get_reference_voltage();
        FIXED_Q1616 reference_voltage =
            FIXED_from_fraction((int32_t)ref_voltage_mv, SI_MILLI_DIV);

        // Convert raw ADC value to voltage using fixed-point arithmetic
        // ADC is 12-bit with oversampling, so max value depends on oversampling
        // ratio
        uint32_t max_value = (4095U * handle->config.oversampling_ratio);

        // voltage = (raw_value * reference_voltage) / max_value
        *voltage_out = FIXED_div(
            FIXED_mul(handle->adc_value, reference_voltage), (int32_t)max_value
        );

        LOG_DEBUG(
            "DMM: Channel %d voltage = %d.%04d V (raw = %u)",
            handle->config.channel,
            FIXED_get_integer_part(*voltage_out),
            (FIXED_get_fractional_part(*voltage_out) * 10000) >> 16,
            handle->adc_value
        );

        // Reset flag and restart ADC for next conversion
        // Timer remains running continuously
        handle->conversion_complete = false;

        Error error = ERROR_NONE;
        TRY
        {
            ADC_LL_start(); // Restart ADC/DMA for next conversion
        }
        CATCH(error)
        {
            LOG_ERROR(
                "DMM: Failed to restart ADC for next conversion with error %d",
                error
            );
            // Don't throw here - return the valid measurement but log the error
        }
    } else {
        // No new conversion available, set voltage to 0
        *voltage_out = FIXED_ZERO;
        LOG_DEBUG("DMM: No new conversion available");
    }

    LOG_FUNCTION_EXIT();
    return conversion_ready;
}
