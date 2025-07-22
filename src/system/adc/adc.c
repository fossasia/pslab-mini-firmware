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
#include "adc.h"
#include "adc_ll.h"
#include "error.h"
#include "logging.h"

/**********************************************************************
 * Macros
 **********************************************************************/

enum { ADC_TIM_NUM = 0 }; // Timer used for ADC conversions
enum { ADC_TIM_Frequency = 25000 }; // ADC timer frequency in Hz
enum { ADC_TRIGGER_TIMER = 6 };

struct ADC_Handle {
    ADC_Num adc_id; // ADC instance ID
    uint32_t *adc_buffer; // Buffer for ADC data
    uint32_t adc_buffer_size; // Size of the ADC buffer
    ADC_Callback g_adc_callback; // Callback for ADC completion
    bool initialized; // Flag to indicate if ADC is initialized
};

static ADC_Handle *g_active_adc_handles[ADC_COUNT] = {
    nullptr
}; // Global ADC handles

/**
 * @brief Callback invoked by hardware layer when ADC conversion is complete
 */
static void adc_complete_callback(ADC_Num adc_num)
{
    if (!g_active_adc_handles[adc_num] ||
        !g_active_adc_handles[adc_num]->initialized) {
        return;
    }

    // Check if we should run the ADC callback now
    g_active_adc_handles[adc_num]->g_adc_callback(g_active_adc_handles[adc_num]
    ); // Call the user-defined callback with the ADC value
}

/**
 * @brief Initializes the ADC peripheral.
 *
 * This function initializes the ADC peripheral with the specified settings.
 * It must be called before any ADC operations can be performed.
 *
 */
void *ADC_init(size_t adc, uint32_t *adc_buffer, uint32_t adc_buffer_size)
{
    if (adc >= ADC_COUNT) {
        THROW(ERROR_INVALID_ARGUMENT);
        return nullptr;
    }
    if (!adc_buffer || adc_buffer_size == 0) {
        THROW(ERROR_INVALID_ARGUMENT);
        return nullptr;
    }

    // Initialize the timer used for ADC conversions
    TIM_init(
        ADC_TIM_NUM, ADC_TIM_Frequency
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

    if (g_active_adc_handles[adc] && g_active_adc_handles[adc]->initialized) {
        THROW(ERROR_RESOURCE_BUSY);
        return nullptr;
    }
    ADC_Handle *handle = malloc(sizeof(ADC_Handle));

    if (!handle) {
        THROW(ERROR_OUT_OF_MEMORY);
        return nullptr;
    }
    // Initialize the ADC handle
    handle->adc_id = (ADC_Num)adc; // Set the ADC instance ID
    handle->adc_buffer = adc_buffer;
    handle->adc_buffer_size = adc_buffer_size;
    handle->g_adc_callback = nullptr;
    handle->initialized = false;
    // Initialize the ADC peripheral
    ADC_LL_init(
        handle->adc_id,
        handle->adc_buffer,
        handle->adc_buffer_size,
        ADC_TRIGGER_TIMER
    );
    // Set the ADC complete callback
    ADC_LL_set_complete_callback(handle->adc_id, adc_complete_callback);
    handle->initialized = true;
    g_active_adc_handles[adc] = handle;

    return handle; // Return the ADC handle
}

/**
 * @brief Deinitializes the ADC peripheral.
 *
 * This function deinitializes the ADC peripheral and releases any resources
 * used. It should be called when the ADC is no longer needed.
 */
void ADC_deinit(ADC_Handle *handle)
{
    TIM_stop(ADC_TIM_NUM); // Stop the timer used for ADC conversions

    if (handle && handle->initialized) {
        // Deinitialize the ADC peripheral
        ADC_LL_deinit(handle->adc_id);

        ADC_LL_set_complete_callback(
            handle->adc_id, nullptr
        ); // Clear the callback

        handle->initialized = false;

        free(handle); // Free the ADC handle memory
    }

    else {
        THROW(ERROR_INVALID_ARGUMENT);
        return;
    }
}

/**
 * @brief Starts the ADC conversion.
 *
 * This function starts the ADC conversion process. It must be called after
 * the ADC has been initialized and configured.
 *
 */
void ADC_start(ADC_Handle *handle)
{
    if (!handle || !handle->initialized) {
        THROW(ERROR_DEVICE_NOT_READY);
        return;
    }

    TIM_start(ADC_TIM_NUM); // Start the timer for ADC conversions

    // Start the ADC conversion
    ADC_LL_start(handle->adc_id);
    LOG_INFO("ADC started successfully");
}

void ADC_restart(ADC_Handle *handle)
{
    if (!handle || !handle->initialized) {
        THROW(ERROR_DEVICE_NOT_READY);
        return;
    }

    ADC_LL_start(handle->adc_id); // Start a new conversion
}

/**
 * @brief Stops the ADC conversion.
 *
 * This function stops the ongoing ADC conversion process. It can be called
 * to halt conversions before deinitializing the ADC or when no longer needed.
 *
 */
void ADC_stop(ADC_Handle *handle)
{
    if (!handle || !handle->initialized) {
        THROW(ERROR_DEVICE_NOT_READY);
        return;
    }

    LOG_INFO("Stopping ADC conversion");

    // Stop the timer used for ADC conversions
    TIM_stop(ADC_TIM_NUM); // Stop the timer used for ADC conversions
    // Stop the ADC conversion
    ADC_LL_stop(handle->adc_id);
}

uint32_t ADC_read(ADC_Handle *handle, uint32_t *const adc_buf, uint32_t sz)
{
    if (!handle || !handle->initialized) {
        THROW(ERROR_DEVICE_NOT_READY);
        return 0;
    }

    if (!adc_buf || sz == 0) {
        THROW(ERROR_INVALID_ARGUMENT);
        return 0;
    }

    uint32_t samples_read = 0;
    while (samples_read < sz) {
        adc_buf[samples_read] = handle->adc_buffer[samples_read];
        samples_read++;
    }

    return samples_read; // Return the number of samples read
}

void ADC_set_callback(ADC_Handle *handle, ADC_Callback callback)
{
    if (!handle || !handle->initialized) {
        THROW(ERROR_INVALID_ARGUMENT);
        return;
    }
    // Set the ADC callback
    handle->g_adc_callback = callback;
}
