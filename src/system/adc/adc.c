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

#include "../timer/tim.h"
#include "adc.h"
#include "adc_ll.h"

/**********************************************************************
 * Macros
 **********************************************************************/

enum { ADC1_TIM = 0 }; // Timer used for ADC1 conversions
enum { ADC1_TIM_Frequency = 25000 }; // ADC1 timer frequency in Hz

/**
 * @brief Initializes the ADC peripheral.
 *
 * This function initializes the ADC peripheral with the specified settings.
 * It must be called before any ADC operations can be performed.
 *
 */
void ADC_init(void)
{
    // Initialize the timer used for ADC conversions
    TIM_Init(
        ADC1_TIM, ADC1_TIM_Frequency
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
    // Initialize the ADC peripheral
    ADC_LL_init();
}

/**
 * @brief Deinitializes the ADC peripheral.
 *
 * This function deinitializes the ADC peripheral and releases any resources
 * used. It should be called when the ADC is no longer needed.
 */
void ADC_deinit(void)
{
    TIM_Stop(ADC1_TIM); // Stop the timer used for ADC conversions
    // Deinitialize the ADC peripheral
    ADC_LL_deinit();
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
    // Start the ADC conversion
    ADC_LL_start();
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
    // Stop the ADC conversion
    ADC_LL_stop();
}

/**
 * @brief Reads the converted ADC value.
 * This function reads the converted ADC value from the specified channel.
 *
 * @param data Pointer to store the converted value.
 */
uint32_t ADC_read(uint32_t *data)
{
    *data = ADC_LL_read(data);

    return *data;
}
