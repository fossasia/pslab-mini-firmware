/**
 * @file adc.c
 * @brief Hardware-independent ADC (Analog-to-Digital Converter) implementation
 *
 * This module provides the hardware-independent layer of the ADC driver.
 * It manages ADC initialization, configuration, and data acquisition.
 * The implementation relies on hardware-specific functions defined in
 * src/system/h563xx/adc_ll.c (or equivalent for other platforms).
 *
 * @author Tejas Garg
 * @date 2025-07-02
 */

#include <stdint.h>
#include <stdlib.h>

#include "../timer/tim.h"
#include "../util/util.h"
#include "adc.h"
#include "adc_ll.h"
#include "error.h"
#include "logging.h"

/**********************************************************************
 * Macros
 **********************************************************************/

enum { ADC1_TIM_NUM = 0 }; // Timer used for ADC1 conversions
enum { ADC1_TIM_Frequency = 25000 }; // ADC1 timer frequency in Hz
enum { ADC1_TRIGGER_TIMER = 6 };

struct ADC_Handle {
    CircularBuffer *adc_buffer; // Circular buffer for ADC data
    uint32_t volatile adc_dma_head; // DMA head position for ADC
    ADC_Callback g_adc_callback; // Callback for ADC completion
    uint32_t adc_threshold; // Threshold for ADC callback
    bool initialized; // Flag to indicate if ADC is initialized
};

static ADC_Handle *g_active_adc_handle = nullptr; // Global ADC handle

static uint32_t adc_buffer_available(ADC_Handle *handle)
{
    if (!handle || !handle->initialized) {
        return 0;
    }

    uint32_t dma_pos = ADC_LL_get_dma_position();
    uint32_t dma_head = dma_pos;

    // Update the DMA head position in the handle
    handle->adc_dma_head = dma_head;
    // Circular buffer head position is updated
    handle->adc_buffer->head = dma_head;

    return circular_buffer_available(handle->adc_buffer);
}

static bool check_adc_callback(ADC_Handle *handle)
{
    if (!handle || !handle->initialized) {
        return false;
    }

    if (!handle->g_adc_callback) {
        return false;
    }

    if (adc_buffer_available(handle) < handle->adc_threshold) {
        return false;
    }

    handle->g_adc_callback(handle, adc_buffer_available(handle));
    return true;
}

static void adc_complete_callback(uint32_t dma_pos)
{
    if (!g_active_adc_handle || !g_active_adc_handle->initialized) {
        return;
    }
    (void)dma_pos;
    // Update the DMA position and circular buffer head
    g_active_adc_handle->adc_dma_head = 0;
    g_active_adc_handle->adc_buffer->head = 0;

    // Check if we should run the ADC callback now
    check_adc_callback(g_active_adc_handle);
}

static void adc_half_complete_callback(uint32_t dma_pos)
{
    if (!g_active_adc_handle || !g_active_adc_handle->initialized) {
        return;
    }

    // review
    g_active_adc_handle->adc_dma_head = dma_pos;
    g_active_adc_handle->adc_buffer->head = dma_pos;

    // Check if we should run the ADC callback now
    check_adc_callback(g_active_adc_handle);
}

/**
 * @brief Initializes the ADC peripheral.
 *
 * This function initializes the ADC peripheral with the specified settings.
 * It must be called before any ADC operations can be performed.
 *
 */
void *ADC_init(CircularBuffer *adc_buffer)
{
    if (!adc_buffer) {
        THROW(ERROR_INVALID_ARGUMENT);
        return nullptr;
    }

    // Initialize the timer used for ADC conversions
    TIM_init(
        ADC1_TIM_NUM, ADC1_TIM_Frequency
    ); // Set the timer frequency to 25000 Hz to enable sampling at 1KSPS
    /*
    Explanation for 25khz to achieve 1KSPS:
    The peripheral clock for the ADC is set to 250MHz.
    and at 25khz of timer frequency, the period and prescaler values are
    calculated as follows:
    - Prescaler = Default Prescaler Value = 0
    - Period = 10000 - 1 = 9999

    Also the ADC is running at 25khz
    and for 12 bit conversion, the ADC takes 12.5 clock cycles to complete SAR.
    and another 12.5 clock cycles for sampling time.
    So, the total time taken for one conversion is 25 clock cycles.
    Therefore, the ADC can perform 25000 / 25 = 1000 conversions per second
    (1KSPS). Hence, the timer frequency is set to 25khz to achieve 1KSPS
    sampling rate. This allows the ADC to sample at a rate of 1KSPS (1000
    samples per second). The timer is used to trigger the ADC conversions at
    this rate. The ADC will be configured to use this timer for triggering
    conversions.
    */

    if (g_active_adc_handle) {
        THROW(ERROR_RESOURCE_BUSY);
        return nullptr;
    }
    ADC_Handle *handle = malloc(sizeof(ADC_Handle));

    if (!handle) {
        THROW(ERROR_OUT_OF_MEMORY);
        return nullptr;
    }
    // Initialize the ADC handle
    handle->adc_buffer = adc_buffer;
    handle->adc_dma_head = 0;
    handle->g_adc_callback = nullptr;
    handle->adc_threshold = 0;
    handle->initialized = false;
    // Initialize the ADC peripheral
    ADC_LL_init(adc_buffer->buffer, adc_buffer->size, ADC1_TRIGGER_TIMER);
    // Set the ADC complete callback
    ADC_LL_set_complete_callback(adc_complete_callback);
    // Set the ADC half-complete callback
    ADC_LL_set_half_complete_callback(adc_half_complete_callback);

    handle->initialized = true;
    g_active_adc_handle = handle;

    return handle; // Return the ADC handle
}

/**
 * @brief Deinitializes the ADC peripheral.
 *
 * This function deinitializes the ADC peripheral and releases any resources
 * used. It should be called when the ADC is no longer needed.
 */
void ADC_deinit(void)
{
    TIM_stop(ADC1_TIM_NUM); // Stop the timer used for ADC conversions
    // Deinitialize the ADC peripheral
    ADC_LL_deinit();
    ADC_LL_set_complete_callback(nullptr);
    ADC_LL_set_half_complete_callback(nullptr);

    g_active_adc_handle->initialized = false;

    free(g_active_adc_handle); // Free the ADC handle memory

    // Clear the global ADC handle
    g_active_adc_handle = nullptr;
}

/**
 * @brief Starts the ADC conversion.
 *
 * This function starts the ADC conversion process. It must be called after
 * the ADC has been initialized and configured.
 *
 */
void ADC_start(void)
{

    TIM_start(ADC1_TIM_NUM); // Start the timer for ADC conversions

    // Start the ADC conversion
    ADC_LL_start();
    LOG_INFO("ADC_started_successfully");
}

/**
 * @brief Stops the ADC conversion.
 *
 * This function stops the ongoing ADC conversion process. It can be called
 * to halt conversions before deinitializing the ADC or when no longer needed.
 *
 */
void ADC_stop(void)
{
    TIM_stop(ADC1_TIM_NUM); // Stop the timer used for ADC conversions
    // Stop the ADC conversion
    ADC_LL_stop();
}

uint32_t ADC_read(uint8_t *const adc_buf, uint32_t sz)
{
    if (!g_active_adc_handle || !g_active_adc_handle->initialized) {
        THROW(ERROR_DEVICE_NOT_READY);
        return 0;
    }

    if (!adc_buf || sz == 0) {
        THROW(ERROR_INVALID_ARGUMENT);
        return 0;
    }

    uint32_t available = adc_buffer_available(g_active_adc_handle);
    uint32_t to_read = sz > available ? available : sz;

    return circular_buffer_read(
        g_active_adc_handle->adc_buffer, adc_buf, to_read
    );
}

void ADC_set_callback(ADC_Callback callback, uint32_t threshold)
{
    if (!g_active_adc_handle || threshold == 0) {
        THROW(ERROR_INVALID_ARGUMENT);
        return;
    }

    // If the ADC is not initialized, set the global active handle
    if (g_active_adc_handle == nullptr) {
        THROW(ERROR_DEVICE_NOT_READY);
        return;
    }

    // Set the ADC callback and threshold
    g_active_adc_handle->g_adc_callback = callback;
    g_active_adc_handle->adc_threshold = threshold;

    if (g_active_adc_handle->g_adc_callback &&
        adc_buffer_available(g_active_adc_handle) >=
            g_active_adc_handle->adc_threshold) {
        // If the callback is set and the buffer has enough data, call the
        // callback
        g_active_adc_handle->g_adc_callback(
            g_active_adc_handle, adc_buffer_available(g_active_adc_handle)
        );
    }
}
