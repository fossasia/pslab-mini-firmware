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
    ADC_TRIGGER_TIMER1 = 1,
    ADC_TRIGGER_TIMER1_TRGO2 = 11, // TIM1 TRGO2
    ADC_TRIGGER_TIMER2 = 2,
    ADC_TRIGGER_TIMER3 = 3,
    ADC_TRIGGER_TIMER4 = 4,
    ADC_TRIGGER_TIMER6 = 6,
    ADC_TRIGGER_TIMER8 = 8,
    ADC_TRIGGER_TIMER8_TRGO2 = 18, // TIM8 TRGO2
    ADC_TRIGGER_TIMER15 = 15
} ADC_LL_TriggerSource;

typedef enum {
    ADC_LL_CHANNEL_0 = 0,
    ADC_LL_CHANNEL_1 = 1,
    ADC_LL_CHANNEL_2 = 2,
    ADC_LL_CHANNEL_3 = 3,
    ADC_LL_CHANNEL_4 = 4,
    ADC_LL_CHANNEL_5 = 5,
    ADC_LL_CHANNEL_6 = 6,
    ADC_LL_CHANNEL_7 = 7,
    ADC_LL_CHANNEL_8 = 8,
    ADC_LL_CHANNEL_9 = 9,
    ADC_LL_CHANNEL_10 = 10,
    ADC_LL_CHANNEL_11 = 11,
    ADC_LL_CHANNEL_12 = 12,
    ADC_LL_CHANNEL_13 = 13,
    ADC_LL_CHANNEL_14 = 14,
    ADC_LL_CHANNEL_15 = 15
} ADC_LL_Channel;

/**
 * @brief ADC configuration structure
 */
typedef struct {
    ADC_LL_Channel channel; // ADC channel to use
    ADC_LL_TriggerSource
        trigger_source; // Timer trigger source
    uint16_t *output_buffer; // Output buffer for DMA transfer
    uint32_t buffer_size; // Buffer size (number of samples)
    uint32_t oversampling_ratio; // Oversampling ratio (1, 2, 4, 8, 16, 32, 64,
                                 // 128, 256)
} ADC_LL_Config;

/**
 * @brief Callback function type for ADC complete events.
 *
 * This callback is called when the ADC conversion is complete.
 * The callback is invoked when the DMA buffer is full.
 */
typedef void (*ADC_LL_CompleteCallback)(void);

/**
 * @brief Initializes the ADC1 peripheral.
 *
 * This function configures the ADC1 peripheral with the specified settings.
 * The ADC uses timer-triggered conversions with DMA for all operations.
 * Oversampling is available for all configurations.
 *
 * @param config Pointer to ADC configuration structure.
 */
void ADC_LL_init(ADC_LL_Config const *config);

/**
 * @brief Starts an ADC conversion.
 *
 * Starts timer-triggered conversions with DMA.
 */
void ADC_LL_start(void);

/**
 * @brief Deinitializes the ADC peripheral.
 *
 * This function deinitializes the ADC peripheral and releases any resources
 * used.
 */
void ADC_LL_deinit(void);

/**
 * @brief Stops the ADC conversion.
 *
 * This function stops the ongoing ADC conversion process. It can be called
 * to halt conversions before deinitializing the ADC or when no longer needed.
 */
void ADC_LL_stop(void);

/**
 * @brief Sets the callback for ADC completion events.
 *
 * This function sets a user-defined callback that will be called when
 * the ADC conversion is complete.
 *
 * @param callback Pointer to the callback function to be set.
 */
void ADC_LL_set_complete_callback(ADC_LL_CompleteCallback callback);

/**
 * @brief Gets the current ADC sample rate.
 *
 * This function calculates and returns the current ADC sample rate based on:
 * - Sample time (currently 12.5 clock cycles)
 * - Conversion time (12.5 clock cycles for 12-bit resolution)
 * - ADC clock frequency
 * - Clock prescaler (currently 1)
 *
 * Formula: Sample Rate = ADC_Clock_Rate / ((Sample_Time + Conversion_Time) * Prescaler)
 *
 * @return The sample rate in Hz, or 0 if ADC is not initialized or error occurs.
 */
uint32_t ADC_LL_get_sample_rate(void);

#endif // ADC_LL_H
