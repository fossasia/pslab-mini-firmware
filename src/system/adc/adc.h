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
 * @brief ADC Structure with parameters and callback.
 */
typedef struct ADC_Handle ADC_Handle;

/**
 * @brief Callback function type for ADC completion.
 *
 * This callback is called when an ADC conversion is complete.
 * It receives the converted ADC value as an argument.
 */
typedef void (*ADC_Callback)(ADC_Handle *handle);

/**
 * @brief ADC Initialization function.
 *
 * This function initializes the ADC peripheral with the specified settings.
 * It must be called before any ADC operations can be performed.
 *
 * @param adc_buffer Pointer to the circular buffer for ADC data.
 */
void *ADC_init(size_t adc, uint16_t *adc_buffer, uint32_t buffer_size);

/**
 * @brief Deinitializes the ADC peripheral.
 *
 * This function deinitializes the ADC peripheral and releases any resources
 * used. It should be called when the ADC is no longer needed.
 */
void ADC_deinit(ADC_Handle *handle);

/**
 * @brief Starts the ADC conversion.
 *
 * This function starts the ADC conversion process. It must be called after
 * the ADC has been initialized and configured.
 */
void ADC_start(ADC_Handle *handle);

/**
 * @brief Restarts the ADC conversion.
 *
 * This function restarts the ADC conversion process. It can be called to
 * restart conversions after dma buffer is completely filled and we need
 * to call the dma to restart the transfer.
 */
void ADC_restart(ADC_Handle *handle);

/**
 * @brief Stops the ADC conversion.
 *
 * This function stops the ongoing ADC conversion process. It can be called
 * to halt conversions before deinitializing the ADC or when no longer needed.

 */
void ADC_stop(ADC_Handle *handle);

/**
 * @brief Reads data from the ADC buffer.
 *
 * This function reads data from the ADC circular buffer into the provided
 * buffer. It checks if the ADC is initialized and if the input buffer is valid.
 *
 * @param adc_buf Pointer to the buffer where ADC data will be stored.
 * @param sz Size of the buffer in bytes.
 *
 * @return Number of bytes read from the ADC buffer.
 */
uint32_t ADC_read(ADC_Handle *handle, uint16_t *adc_buf, uint32_t sz);

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
void ADC_set_callback(ADC_Handle *handle, ADC_Callback callback);

#ifdef __cplusplus
}
#endif

#endif // ADC_H
