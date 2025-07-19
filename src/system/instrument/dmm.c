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
#include "tim_ll.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

/**
 * @brief DMM handle structure
 */
struct DMM_Handle {
    DMM_Config config;
    uint16_t adc_buffer; // Single value buffer for single sample mode
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
        case DMM_CHANNEL_0: return ADC_LL_CHANNEL_0;
        case DMM_CHANNEL_1: return ADC_LL_CHANNEL_1;
        case DMM_CHANNEL_2: return ADC_LL_CHANNEL_2;
        case DMM_CHANNEL_3: return ADC_LL_CHANNEL_3;
        case DMM_CHANNEL_4: return ADC_LL_CHANNEL_4;
        case DMM_CHANNEL_5: return ADC_LL_CHANNEL_5;
        case DMM_CHANNEL_6: return ADC_LL_CHANNEL_6;
        case DMM_CHANNEL_7: return ADC_LL_CHANNEL_7;
        case DMM_CHANNEL_8: return ADC_LL_CHANNEL_8;
        case DMM_CHANNEL_9: return ADC_LL_CHANNEL_9;
        case DMM_CHANNEL_10: return ADC_LL_CHANNEL_10;
        case DMM_CHANNEL_11: return ADC_LL_CHANNEL_11;
        case DMM_CHANNEL_12: return ADC_LL_CHANNEL_12;
        case DMM_CHANNEL_13: return ADC_LL_CHANNEL_13;
        case DMM_CHANNEL_14: return ADC_LL_CHANNEL_14;
        case DMM_CHANNEL_15: return ADC_LL_CHANNEL_15;
        default:
            // This should never happen due to validation, but satisfies linter
            return ADC_LL_CHANNEL_0;
    }
}

/**
 * @brief ADC conversion complete callback
 *
 * @note This function has extern linkage to facilitate testing. It is not part
 * of the public API.
 */
void dmm_adc_complete_callback(void)
{
    if (g_dmm_handle != NULL) {
        g_dmm_handle->conversion_complete = true;
        LOG_DEBUG(
            "DMM: ADC conversion complete, value = %u", g_dmm_handle->adc_buffer
        );
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

    // Validate timeout
    if (config->timeout_ms == 0) {
        LOG_ERROR("DMM: Timeout must be greater than 0");
        return false;
    }

    // Validate reference voltage
    if (config->reference_voltage <= FIXED_ZERO ||
        config->reference_voltage > FIXED_FROM_FLOAT(5.0f)) {
        LOG_ERROR(
            "DMM: Invalid reference voltage: %d.%04d V",
            fixed_get_integer_part(config->reference_voltage),
            (fixed_get_fractional_part(config->reference_voltage) * 10000) >> 16
        );
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
    handle->adc_buffer = 0;
    handle->conversion_complete = false;
    handle->initialized = true;
    g_dmm_handle = handle;

    LOG_INFO(
        "DMM: Initializing with channel %d, oversampling ratio %u, timeout %u ms, "
        "reference %d.%04d V",
        config->channel,
        config->oversampling_ratio,
        config->timeout_ms,
        fixed_get_integer_part(config->reference_voltage),
        (fixed_get_fractional_part(config->reference_voltage) * 10000) >> 16
    );

    // Set up ADC callback
    ADC_LL_set_complete_callback(dmm_adc_complete_callback);

    // Initialize ADC with the configured channel
    ADC_LL_Config adc_config = {
        .channel = dmm_channel_to_adc_ll(config->channel),
        .trigger_source = ADC_TRIGGER_TIMER6,
        .output_buffer = &handle->adc_buffer,
        .buffer_size = 1,  // Single sample
        .oversampling_ratio = handle->config.oversampling_ratio
    };

    Error error = ERROR_NONE;
    TRY {
        ADC_LL_init(&adc_config);
    }
    CATCH(error) {
        LOG_ERROR("DMM: ADC initialization failed with error %d", error);
        g_dmm_handle = NULL;
        free(handle);
        THROW(error);
    }

    // Initialize timer for ADC triggering
    TRY
    {
        TIM_LL_init(TIM_NUM_0, ADC_LL_get_sample_rate());
    }
    CATCH(error)
    {
        LOG_ERROR("DMM: Timer initialization failed with error %d", error);
        ADC_LL_deinit();
        g_dmm_handle = NULL;
        free(handle);
        THROW(error);
    }
    LOG_DEBUG("DMM: Timer initialized with frequency %u Hz", ADC_LL_get_sample_rate());

    LOG_INFO("DMM: Initialized successfully");
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

    // Deinitialize ADC
    ADC_LL_deinit();

    // Clear global handle
    g_dmm_handle = NULL;

    // Mark as deinitialized
    handle->initialized = false;

    // Free memory
    free(handle);

    LOG_INFO("DMM: Deinitialized successfully");
    LOG_FUNCTION_EXIT();
}

static void read_raw(
    DMM_Handle *handle,
    uint16_t *raw_value_out
)
{
    LOG_FUNCTION_ENTRY();

    if (handle == NULL || raw_value_out == NULL) {
        LOG_ERROR(
            "DMM: Invalid arguments (handle=%p, raw_value_out=%p)",
            handle,
            raw_value_out
        );
        THROW(ERROR_INVALID_ARGUMENT);
    }

    if (!handle->initialized) {
        LOG_ERROR("DMM: Handle not initialized");
        THROW(ERROR_DEVICE_NOT_READY);
    }

    LOG_DEBUG("DMM: Reading raw value from channel %d", handle->config.channel);

    // Reset conversion complete flag
    handle->conversion_complete = false;

    // Start conversion
    Error error = ERROR_NONE;
    TRY {
        TIM_LL_start(TIM_NUM_0); // Start timer for ADC triggering
        ADC_LL_start();
    }
    CATCH(error)
    {
        LOG_ERROR("DMM: ADC start failed with error %d", error);
        THROW(error);
    }

    // Wait for conversion to complete with timeout
    uint32_t timeout_count = 0;
    uint32_t const max_timeout =
        handle->config.timeout_ms * 1000; // Convert to microseconds

    while (!handle->conversion_complete && timeout_count < max_timeout) {
        // Simple delay loop (in real implementation, you might want to use a
        // proper delay)
        for (int volatile i = 0; i < 100; i++); // ~1μs delay
        timeout_count++;
    }

    if (!handle->conversion_complete) {
        LOG_ERROR(
            "DMM: Conversion timeout after %u ms", handle->config.timeout_ms
        );
        ADC_LL_stop();
        TIM_LL_stop(TIM_NUM_0);
        // Note: Don't deinitialize ADC here, keep it ready for next measurement
        THROW(ERROR_TIMEOUT);
    }

    // Get the result
    *raw_value_out = handle->adc_buffer;

    // Stop current conversion but keep ADC initialized for next measurement
    ADC_LL_stop();
    TIM_LL_stop(TIM_NUM_0);

    LOG_DEBUG("DMM: Raw value from channel %d = %u", handle->config.channel, *raw_value_out);
    LOG_FUNCTION_EXIT();
}

void DMM_read_voltage(
    DMM_Handle *handle,
    Fixed *voltage_out
)
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

    uint16_t raw_value;
    read_raw(handle, &raw_value);

    // Convert raw ADC value to voltage using fixed-point arithmetic
    // ADC is 16-bit with oversampling, so max value depends on oversampling
    // ratio
    uint32_t max_value = (65535U * handle->config.oversampling_ratio);

    // voltage = (raw_value * reference_voltage) / max_value
    // Use 64-bit intermediate to avoid overflow
    int64_t temp =
        ((int64_t)raw_value * (int64_t)handle->config.reference_voltage);
    *voltage_out = (Fixed)(temp / (int64_t)max_value);

    LOG_DEBUG(
        "DMM: Channel %d voltage = %d.%04d V (raw = %u)",
        handle->config.channel,
        fixed_get_integer_part(*voltage_out),
        (fixed_get_fractional_part(*voltage_out) * 10000) >> 16,
        raw_value
    );
    LOG_FUNCTION_EXIT();
}
