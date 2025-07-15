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

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Callback function type for ADC completion.
 *
 * This callback is called when an ADC conversion is complete.
 * It receives the converted ADC value as an argument.
 */
typedef void (*ADC_CompleteCallback)(uint32_t value);

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
 * @brief Sets the callback function to be called when an ADC conversion is
 * complete.
 *
 * This function allows the user to set a callback that will be invoked
 * when an ADC conversion is complete. The callback will receive the converted
 * ADC value as an argument.
 *
 * @param callback Pointer to the callback function to be set.
 */
void ADC_set_complete_callback(ADC_CompleteCallback callback);

#ifdef __cplusplus
}
#endif

#endif // ADC_H
