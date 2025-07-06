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

#include "adc.h"
#include "adc_ll.h"

/**
 * @brief Initializes the ADC peripheral.
 *
 * This function initializes the ADC peripheral with the specified settings.
 * It must be called before any ADC operations can be performed.
 *
 */
void ADC_init(void)
{
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
