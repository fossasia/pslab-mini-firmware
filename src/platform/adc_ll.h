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

#define MAX_SIMULTANEOUS_CHANNELS 2

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

typedef enum {
    ADC_LL_MODE_SINGLE = 0, // Single ADC operation
    ADC_LL_MODE_SIMULTANEOUS, // Simultaneous sampling on ADC1 and ADC2
    ADC_LL_MODE_INTERLEAVED // Interleaved sampling on ADC1 and ADC2
} ADC_LL_Mode;

/**
 * @brief ADC configuration structure
 */
typedef struct {
    ADC_LL_Channel channels[MAX_SIMULTANEOUS_CHANNELS]; // ADC channels to use
    ADC_LL_Mode mode; // ADC operation mode
    ADC_LL_TriggerSource trigger_source; // Timer trigger source
    uint16_t *output_buffer; // Output buffer for DMA transfer
    uint32_t buffer_size; // Buffer size (number of samples per channel)
    uint32_t oversampling_ratio; // Oversampling ratio (1, 2, 4, 8, 16, 32, 64,
                                 // 128, 256)
} ADC_LL_Config;

/**
 * @brief Callback function type for ADC complete events.
 *
 * This callback is called when the ADC conversion is complete and the buffer
 * is full.
 *
 * Data Format:
 * - Single mode: Buffer contains samples from one ADC channel
 * - Simultaneous mode: Buffer contains simultaneous sample pairs from both
 *   ADCs / channels
 *   [ADC1_sample0, ADC2_sample0, ADC1_sample1, ADC2_sample1, ...]
 *   Total buffer size is 2 * buffer_size samples
 * - Interleaved mode: Buffer contains time-sequential samples from one channel
 *   alternating between ADCs
 *   [ADC1_sample0, ADC2_sample1, ADC1_sample2, ADC2_sample3, ...]
 *   Total buffer size is 2 * buffer_size samples
 *
 * @param buffer Pointer to the output buffer
 * @param total_samples Total number of samples in the buffer
 */
typedef void (*ADC_LL_CompleteCallback)(
    uint16_t *buffer,
    uint32_t total_samples
);

/**
 * @brief Initialize the ADC peripheral(s).
 *
 * This function configures the ADC peripheral(s) with the specified settings.
 * For single-channel mode, only ADC1 is used. For simultaneous and interleaved
 * modes, both ADC1 and ADC2 are configured.
 *
 * Buffer Requirements:
 * - Single mode: Buffer accommodates buffer_size samples
 * - Simultaneous mode: Buffer accommodates 2 * buffer_size samples
 * - Interleaved mode: Buffer accommodates 2 * buffer_size samples
 *
 * @param config Pointer to ADC configuration structure.
 */
void ADC_LL_init(ADC_LL_Config const *config);

/**
 * @brief Start ADC conversion(s).
 *
 * Starts timer-triggered conversions with DMA for the configured ADC(s).
 */
void ADC_LL_start(void);

/**
 * @brief Deinitialize the ADC peripheral(s).
 *
 * This function deinitializes the ADC peripheral(s) and releases any resources
 * used.
 */
void ADC_LL_deinit(void);

/**
 * @brief Stop the ADC conversion(s).
 *
 * This function stops the ongoing ADC conversion process. It can be called
 * to halt conversions before deinitializing the ADC or when no longer needed.
 */
void ADC_LL_stop(void);

/**
 * @brief Set the callback for ADC completion events.
 *
 * This function sets a user-defined callback that will be called when
 * the ADC conversion is complete.
 *
 * @param callback Pointer to the callback function to be set.
 */
void ADC_LL_set_complete_callback(ADC_LL_CompleteCallback callback);

/**
 * @brief Get the current ADC operation mode.
 *
 * @return Current ADC mode (single, simultaneous, or interleaved).
 */
ADC_LL_Mode ADC_LL_get_mode(void);

/**
 * @brief Get the current ADC sample rate.
 *
 * This function calculates and returns the current ADC sample rate based on:
 * - Sample time
 * - Resolution (affects conversion time)
 * - ADC clock frequency
 * - Clock prescaler
 *
 * Formula: Sample Rate = ADC_Clock_Rate / ((Sample_Time + Conversion_Time) *
 * Prescaler)
 *
 * @return The sample rate in Hz, or 0 if ADC is not initialized or error
 * occurs.
 */
uint32_t ADC_LL_get_sample_rate(void);

/**
 * @brief Get the reference voltage reading.
 *
 * This function returns the reference voltage that was measured during ADC
 * initialization. The value is obtained by reading the internal VREFINT
 * channel and calculating the actual reference voltage using factory
 * calibration data.
 *
 * @return Reference voltage in millivolts, or 0 if ADC is not initialized.
 */
uint32_t ADC_LL_get_reference_voltage(void);

#endif // ADC_LL_H
