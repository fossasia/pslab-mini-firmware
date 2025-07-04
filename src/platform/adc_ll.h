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

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Initializes the ADC1 peripheral.
 *
 * This function configures the ADC1 peripheral with the specified settings.
 */
void ADC1_LL_Init(void);

/**
 * @brief Deinitializes the ADC peripheral.
 *
 * This function deinitializes the ADC peripheral and releases any resources
 * used.
 */
void ADC_LL_DeInit(void);

/**
 * @brief Starts the ADC conversion.
 *
 * This function starts the ADC conversion process. It must be called after
 * the ADC has been initialized and configured.
 */
void ADC1_Start(void);

/**
 * @brief Stops the ADC conversion.
 *
 * This function stops the ongoing ADC conversion process. It can be called
 * to halt conversions before deinitializing the ADC or when no longer needed.
 */
void ADC1_Stop(void);

/**
 * @brief Reads the ADC value into the provided buffer.
 *
 * This function starts the ADC conversion, waits for it to complete,
 * and reads the converted value into the provided buffer.
 *
 * @param buffer Pointer to a buffer where the ADC value will be stored.
 * @return The converted ADC value.
 */
uint32_t ADC_LL_Read(uint32_t *buffer);

#endif // ADC_LL_H
