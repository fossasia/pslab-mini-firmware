/**
 * @file dso.c
 * @brief Digital Storage Oscilloscope implementation for PSLab firmware
 *
 * This file implements a digital storage oscilloscope using the ADC_LL API
 * in continuous sampling mode, supporting both single-channel and dual-channel
 * modes for high-speed data acquisition.
 *
 * @author PSLab Team
 * @date 2025-09-29
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "platform/adc_ll.h"
#include "platform/tim_ll.h"
#include "util/error.h"
#include "util/logging.h"

#include "dso.h"

/**
 * @brief DSO handle structure
 */
struct DSO_Handle {
    DSO_Config config;
    bool running;
};

// Static instance for callback context
static DSO_Handle *g_dso_handle = nullptr;

/**
 * @brief Convert DSO channel to ADC_LL channel
 */
static ADC_LL_Channel dso_channel_to_adc_ll(DSO_Channel channel)
{
    // Map DSO channels to specific ADC channels for oscilloscope functionality
    switch (channel) {
    case DSO_CHANNEL_0:
        return ADC_LL_CHANNEL_0;
    case DSO_CHANNEL_1:
        return ADC_LL_CHANNEL_1;
    default:
        // This should never happen due to validation, but satisfies linter
        return ADC_LL_CHANNEL_0;
    }
}

/**
 * @brief Convert DSO mode to ADC_LL_Mode
 *
 * @param mode DSO mode.
 * @return Corresponding ADC_LL_Mode.
 */
static ADC_LL_Mode dso_mode_to_adc_ll(DSO_Mode mode)
{
    switch (mode) {
    case DSO_MODE_SINGLE_CHANNEL:
        return ADC_LL_MODE_INTERLEAVED;
    case DSO_MODE_DUAL_CHANNEL:
        return ADC_LL_MODE_SIMULTANEOUS;
    default:
        return ADC_LL_MODE_INTERLEAVED;
    }
}

/**
 * @brief ADC completion callback for DSO
 *
 * Called when ADC data acquisition is complete.
 */
// NOLINTNEXTLINE(readability-non-const-parameter)
static void dso_adc_complete_callback(uint16_t *buffer, uint32_t total_samples)
{
    (void)buffer;
    (void)total_samples;

    if (g_dso_handle != nullptr &&
        g_dso_handle->config.complete_callback != nullptr) {
        g_dso_handle->running = false;
        TIM_LL_stop(TIM_NUM_6);
        g_dso_handle->config.complete_callback();
    }
}

/**
 * @brief Validate DSO configuration
 */
static bool dso_validate_config(DSO_Config const *config)
{
    if (config == nullptr) {
        LOG_ERROR("DSO: Configuration is NULL");
        return false;
    }

    // Validate buffer
    if (config->buffer == nullptr) {
        LOG_ERROR("DSO: Buffer is NULL");
        return false;
    }

    if (config->buffer_size == 0) {
        LOG_ERROR("DSO: Buffer size is zero");
        return false;
    }

    // Validate mode and channels
    if (config->mode != DSO_MODE_SINGLE_CHANNEL &&
        config->mode != DSO_MODE_DUAL_CHANNEL) {
        LOG_ERROR("DSO: Invalid mode: %d", config->mode);
        return false;
    }

    // Validate channel for single-channel mode
    if (config->mode == DSO_MODE_SINGLE_CHANNEL) {
        if (config->channel != DSO_CHANNEL_0 &&
            config->channel != DSO_CHANNEL_1) {
            LOG_ERROR("DSO: Invalid channel: %d", config->channel);
            return false;
        }
    }

    // Validate sample rate (basic range check)
    uint32_t max_sample_rate =
        ADC_LL_get_max_sample_rate(dso_mode_to_adc_ll(config->mode));
    if (config->sample_rate == 0 || config->sample_rate > max_sample_rate) {
        LOG_ERROR("DSO: Invalid sample rate: %u", config->sample_rate);
        return false;
    }

    return true;
}

/**
 * @brief Create ADC configuration for DSO based on mode
 */
static ADC_LL_Config dso_create_adc_config(DSO_Handle *handle)
{
    ADC_LL_Config adc_config = { .trigger_source = ADC_TRIGGER_TIMER6 };

    switch (handle->config.mode) {
    case DSO_MODE_SINGLE_CHANNEL:
        adc_config.channels[0] = dso_channel_to_adc_ll(handle->config.channel);
        adc_config.channels[1] = dso_channel_to_adc_ll(handle->config.channel);
        adc_config.mode =
            ADC_LL_MODE_INTERLEAVED; // Use interleaved for higher sample rate
        break;

    case DSO_MODE_DUAL_CHANNEL:
        adc_config.channels[0] = dso_channel_to_adc_ll(DSO_CHANNEL_0);
        adc_config.channels[1] = dso_channel_to_adc_ll(DSO_CHANNEL_1);
        adc_config.mode =
            ADC_LL_MODE_SIMULTANEOUS; // Simultaneous for dual channel
        break;

    default:
        // This should not happen due to validation
        adc_config.channels[0] = ADC_LL_CHANNEL_0;
        adc_config.mode = ADC_LL_MODE_SINGLE;
        break;
    }

    adc_config.trigger_source = ADC_TRIGGER_TIMER6;
    adc_config.output_buffer = handle->config.buffer;
    adc_config.buffer_size = handle->config.buffer_size;
    adc_config.oversampling_ratio = 1; // No oversampling for oscilloscope

    return adc_config;
}

/**
 * @brief Allocate and initialize DSO handle
 */
static DSO_Handle *dso_create_handle(DSO_Config const *config)
{
    // Check if already initialized
    if (g_dso_handle != nullptr) {
        LOG_ERROR("DSO: Already initialized");
        THROW(ERROR_RESOURCE_BUSY);
    }

    // Allocate handle
    DSO_Handle *handle = (DSO_Handle *)malloc(sizeof(DSO_Handle));
    if (handle == nullptr) {
        LOG_ERROR("DSO: Memory allocation failed");
        THROW(ERROR_OUT_OF_MEMORY);
    }

    LOG_DEBUG("DSO: Allocated handle at %p", (void *)handle);

    // Initialize handle
    handle->config = *config;
    handle->running = false;
    g_dso_handle = handle;

    LOG_INFO(
        "DSO: Init mode %d, sample_rate %u, buffer_size %u",
        config->mode,
        config->sample_rate,
        config->buffer_size
    );

    return handle;
}

/**
 * @brief Initialize ADC for DSO operation
 */
static void dso_init_adc(DSO_Handle *handle)
{
    LOG_FUNCTION_ENTRY();

    // Set up ADC callback
    ADC_LL_set_complete_callback(dso_adc_complete_callback);

    LOG_DEBUG("DSO: Configuring ADC");
    ADC_LL_Config adc_config = dso_create_adc_config(handle);

    LOG_DEBUG("DSO: Initializing ADC");
    Error error = ERROR_NONE;
    TRY
    {
        LOG_DEBUG("DSO: ADC_LL_init called");
        ADC_LL_init(&adc_config);
        LOG_DEBUG("DSO: ADC initialized");
    }
    CATCH(error)
    {
        LOG_ERROR("DSO: ADC init failed, error %d", error);
        g_dso_handle = nullptr;
        free(handle);
        THROW(error);
    }
    LOG_FUNCTION_EXIT();
}

/**
 * @brief Initialize timer for ADC triggering
 */
static void dso_init_timer(DSO_Handle *handle)
{
    LOG_FUNCTION_ENTRY();

    Error error = ERROR_NONE;
    TRY { TIM_LL_init(TIM_NUM_6, handle->config.sample_rate); }
    CATCH(error)
    {
        LOG_ERROR("DSO: Timer init failed, error %d", error);
        ADC_LL_deinit();
        g_dso_handle = nullptr;
        free(handle);
        THROW(error);
    }
    LOG_DEBUG("DSO: Timer init, freq %u Hz", handle->config.sample_rate);
    LOG_FUNCTION_EXIT();
}

// Public API Functions

DSO_Handle *DSO_init(DSO_Config const *config)
{
    LOG_FUNCTION_ENTRY();

    // Validate configuration
    if (!dso_validate_config(config)) {
        LOG_ERROR("DSO: Invalid configuration");
        THROW(ERROR_INVALID_ARGUMENT);
    }

    // Create and initialize handle
    DSO_Handle *handle = dso_create_handle(config);

    // Initialize ADC
    dso_init_adc(handle);

    // Initialize timer
    dso_init_timer(handle);

    LOG_INFO("DSO: Successfully initialized");
    LOG_FUNCTION_EXIT();
    return handle;
}

void DSO_deinit(DSO_Handle *handle)
{
    LOG_FUNCTION_ENTRY();

    if (handle == nullptr) {
        LOG_WARN("DSO: Attempted to deinitialize NULL handle");
        return;
    }

    if (handle != g_dso_handle) {
        LOG_ERROR("DSO: Invalid handle");
        THROW(ERROR_INVALID_ARGUMENT);
    }

    Error error = ERROR_NONE;

    TRY
    {
        // Stop if running
        if (handle->running) {
            DSO_stop(handle);
        }

        // Deinitialize hardware
        LOG_DEBUG("DSO: Deinitializing ADC");
        ADC_LL_deinit();

        LOG_DEBUG("DSO: Deinitializing Timer");
        TIM_LL_deinit(TIM_NUM_6);
    }
    CATCH(error)
    {
        LOG_ERROR("DSO: Deinitialization failed, error %d", error);
        // Don't throw, continue with handle cleanup
    }

    // Free memory
    LOG_DEBUG("DSO: Freeing handle at %p", (void *)handle);
    free(handle);

    // Clear global handle reference
    g_dso_handle = nullptr;

    LOG_FUNCTION_EXIT();
}

void DSO_start(DSO_Handle *handle)
{
    LOG_FUNCTION_ENTRY();

    if (handle == nullptr) {
        LOG_ERROR("DSO: Handle is NULL");
        THROW(ERROR_INVALID_ARGUMENT);
    }

    if (handle != g_dso_handle) {
        LOG_ERROR("DSO: Invalid handle");
        THROW(ERROR_INVALID_ARGUMENT);
    }

    if (handle->running) {
        LOG_WARN("DSO: Already running");
        return;
    }

    LOG_DEBUG("DSO: Starting data acquisition");

    Error error = ERROR_NONE;
    TRY
    {
        // Start ADC conversion first (DMA ready but not triggered)
        LOG_DEBUG("DSO: Starting ADC...");
        ADC_LL_start();
        LOG_DEBUG("DSO: ADC started successfully");

        // Start timer to trigger ADC (must be after ADC is ready)
        LOG_DEBUG("DSO: Starting Timer...");
        TIM_LL_start(TIM_NUM_6);
        LOG_DEBUG("DSO: Timer started successfully");

        handle->running = true;
        LOG_INFO("DSO: Data acquisition started");
    }
    CATCH(error)
    {
        LOG_ERROR("DSO: Failed to start, error %d", error);
        // Clean up partial state
        TIM_LL_stop(TIM_NUM_6);
        ADC_LL_stop();
        THROW(error);
    }

    LOG_FUNCTION_EXIT();
}

void DSO_stop(DSO_Handle *handle)
{
    LOG_FUNCTION_ENTRY();

    if (handle == nullptr) {
        LOG_ERROR("DSO: Handle is NULL");
        THROW(ERROR_INVALID_ARGUMENT);
    }

    if (handle != g_dso_handle) {
        LOG_ERROR("DSO: Invalid handle");
        THROW(ERROR_INVALID_ARGUMENT);
    }

    if (!handle->running) {
        LOG_WARN("DSO: Already stopped");
    }

    LOG_DEBUG("DSO: Stopping data acquisition");

    // Stop ADC and timer
    ADC_LL_stop();
    TIM_LL_stop(TIM_NUM_6);

    handle->running = false;

    LOG_INFO("DSO: Data acquisition stopped");
    LOG_FUNCTION_EXIT();
}

DSO_Config DSO_get_config(DSO_Handle *handle)
{
    LOG_FUNCTION_ENTRY();

    if (handle == nullptr) {
        LOG_ERROR("DSO: Handle is NULL");
        THROW(ERROR_INVALID_ARGUMENT);
    }

    if (handle != g_dso_handle) {
        LOG_ERROR("DSO: Invalid handle");
        THROW(ERROR_INVALID_ARGUMENT);
    }

    LOG_DEBUG("DSO: Returning current configuration");
    LOG_FUNCTION_EXIT();
    return handle->config;
}

void DSO_set_config(DSO_Handle *handle, DSO_Config const *config)
{
    LOG_FUNCTION_ENTRY();

    if (handle == nullptr) {
        LOG_ERROR("DSO: Handle is NULL");
        THROW(ERROR_INVALID_ARGUMENT);
    }

    if (handle != g_dso_handle) {
        LOG_ERROR("DSO: Invalid handle");
        THROW(ERROR_INVALID_ARGUMENT);
    }

    if (handle->running) {
        LOG_ERROR("DSO: Cannot update configuration while running");
        THROW(ERROR_RESOURCE_BUSY);
    }

    // Validate new configuration
    if (!dso_validate_config(config)) {
        LOG_ERROR("DSO: Invalid configuration");
        THROW(ERROR_INVALID_ARGUMENT);
    }

    LOG_DEBUG("DSO: Updating configuration");

    Error error = ERROR_NONE;
    TRY
    {
        // Deinitialize current hardware configuration
        LOG_DEBUG("DSO: Deinitializing current hardware");
        ADC_LL_deinit();
        TIM_LL_deinit(TIM_NUM_6);

        // Update configuration
        handle->config = *config;

        // Reinitialize hardware with new configuration
        LOG_DEBUG("DSO: Reinitializing hardware with new config");
        dso_init_adc(handle);
        dso_init_timer(handle);

        LOG_INFO("DSO: Configuration updated successfully");
    }
    CATCH(error)
    {
        LOG_ERROR("DSO: Failed to update configuration, error %d", error);
        THROW(error);
    }

    LOG_FUNCTION_EXIT();
}

uint32_t DSO_get_max_sample_rate(DSO_Mode mode)
{
    LOG_FUNCTION_ENTRY();

    ADC_LL_Mode adc_mode = dso_mode_to_adc_ll(mode);
    uint32_t max_rate = ADC_LL_get_max_sample_rate(adc_mode);

    LOG_DEBUG("DSO: Max sample rate for mode %d: %u Hz", mode, max_rate);
    LOG_FUNCTION_EXIT();

    return max_rate;
}

bool DSO_is_acquisition_in_progress(DSO_Handle *handle)
{
    if (handle == nullptr) {
        LOG_ERROR("DSO: Handle is NULL");
        THROW(ERROR_INVALID_ARGUMENT);
    }

    if (handle != g_dso_handle) {
        LOG_ERROR("DSO: Invalid handle");
        THROW(ERROR_INVALID_ARGUMENT);
    }

    return handle->running;
}
