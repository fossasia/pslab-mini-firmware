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

typedef void (*ADC_LL_CompleteCallback)(uint32_t value);

/**
 * @brief Initializes the ADC1 peripheral.
 *
 * This function configures the ADC1 peripheral with the specified settings.
 */
void ADC_LL_init(ADC_LL_TriggerSource adc_trigger_timer);

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
 * @brief Sets the callback function to be called when an ADC conversion is
 * complete.
 *
 * This function allows the user to set a callback that will be invoked
 * when an ADC conversion is complete.
 *
 * @param callback Pointer to the callback function to be set.
 */
void ADC_LL_set_complete_callback(ADC_LL_CompleteCallback callback);

#endif // ADC_LL_H
