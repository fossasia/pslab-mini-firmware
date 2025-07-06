/**
 * @file adc.h
 * @brief Hardware-independent ADC (Analog-to-Digital Converter) interface
 *
 * This header file defines the public API for the ADC driver.
 * It provides functions for initializing the ADC, configuring channels,
 * starting conversions, and reading ADC values.
 *
 * This implementation relies on hardware-specific functions defined in
 * src/system/h563xx/adc_ll.c (or equivalent for other platforms).
 * @author Tejas Garg
 * @date 2025-07-02
 */

#ifndef ADC_H
#define ADC_H

#include <stdint.h>

#include "adc_ll.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief ADC Initialization function.
 *
 * This function initializes the ADC peripheral with the specified settings.
 * It must be called before any ADC operations can be performed.
 */
void ADC_init(void);

/**
 * @brief Deinitializes the ADC peripheral.
 *
 * This function deinitializes the ADC peripheral and releases any resources
 * used. It should be called when the ADC is no longer needed.
 */
void ADC_deinit(void);

/**
 * @brief Starts the ADC conversion.
 *
 * This function starts the ADC conversion process. It must be called after
 * the ADC has been initialized and configured.
 */
void ADC_start(void);

/**
 * @brief Stops the ADC conversion.
 *
 * This function stops the ongoing ADC conversion process. It can be called
 * to halt conversions before deinitializing the ADC or when no longer needed.

 */
void ADC_stop(void);

/**
 * @brief Reads the ADC value.
 *
 * This function reads the converted ADC value from the specified channel.
 * It should be called after starting the ADC conversion.
 *
 * @param data Pointer to store the read ADC value.
 */
uint32_t ADC_read(uint32_t *data);

#ifdef __cplusplus
}
#endif

#endif // ADC_H
