/**
 * @file adc_ll.h
 * @brief Hardware-specific ADC (Analog-to-Digital Converter) interface
 *
 * This header file defines the hardware-specific interface for the ADC driver.
 *
 * @author Tejas Garg
 * @date 2025-07-02
 */
#ifndef PSLAB_ADC_LL_H
#define PSLAB_ADC_LL_H

#include <stdint.h>

typedef enum {
    ADC_TRIGGER_TIMER6 = 6,
    ADC_TRIGGER_TIMER7 = 7
} ADC_LL_TriggerSource;

/**
 * @brief Callback function type for ADC complete events.
 *
 * This callback is called when the ADC conversion is complete.
 * It receives the DMA position as an argument.
 * @param dma_pos Current DMA position.
 */
typedef void (*ADC_LL_CompleteCallback)(uint32_t dma_pos);

/**
 * @brief Callback function type for ADC half-complete events.
 *
 * This callback is called when the ADC conversion is half complete.
 * It receives the DMA position as an argument.
 *
 * @param dma_pos Current DMA position.
 */
typedef void (*ADC_LL_HalfCompleteCallback)(uint32_t dma_pos);

/**
 * @brief Initializes the ADC1 peripheral.
 *
 * This function configures the ADC1 peripheral with the specified settings.
 * It must be called before any ADC operations can be performed.
 *
 * @param adc_buf Pointer to the ADC data buffer.
 * @param sz Size of the ADC data buffer.
 * @param adc_trigger_timer Trigger source for the ADC (e.g., timer).
 */
void ADC_LL_init(
    uint8_t *adc_buf,
    uint32_t sz,
    ADC_LL_TriggerSource adc_trigger_timer
);

/**
 * @brief Deinitializes the ADC peripheral.
 *
 * This function deinitializes the ADC peripheral and releases any resources
 * used.
 */
void ADC_LL_deinit(void);

/**
 * @brief Starts the ADC conversion.
 *
 * This function starts the ADC conversion process. It must be called after
 * the ADC has been initialized and configured.
 */
void ADC_LL_start(void);

/**
 * @brief Stops the ADC conversion.
 *
 * This function stops the ongoing ADC conversion process. It can be called
 * to halt conversions before deinitializing the ADC or when no longer needed.
 */
void ADC_LL_stop(void);

/**
 * @brief Gets the current DMA position.
 *
 * This function retrieves the current position of the DMA buffer.
 * It returns the number of bytes that have been transferred by the DMA.
 *
 * @return Current DMA position.
 */
uint32_t ADC_LL_get_dma_position(void);

/**
 * @brief Sets the callback for ADC completion events.
 *
 * This function sets a user-defined callback that will be called when
 * the ADC conversion is complete.
 *
 * @param callback Pointer to the callback function to be set.
 */
void ADC_LL_set_complete_callback(ADC_LL_HalfCompleteCallback callback);

/**
 * @brief Sets the callback for ADC half-complete events.
 *
 * This function sets a user-defined callback that will be called when
 * the ADC conversion is half complete.
 *
 * @param callback Pointer to the callback function to be set.
 */
void ADC_LL_set_half_complete_callback(ADC_LL_HalfCompleteCallback callback);

#endif // ADC_LL_H
