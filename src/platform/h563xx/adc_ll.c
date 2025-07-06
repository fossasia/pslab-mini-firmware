/**
 * @file adc_ll.c
 * @brief Hardware-specific ADC (Analog-to-Digital Converter) implementation
 *
 * This module provides the hardware-specific layer of the ADC driver.
 *
 * This implementation relies on the STM32 HAL library for ADC operations.
 * It provides functions to initialize the ADC, configure channels, start
 * conversions, and read ADC values.
 *
 * @author Tejas Garg
 * @date 2025-07-02
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "stm32h5xx_hal.h"

#include "adc_ll.h"

ADC_HandleTypeDef hadc = { 0 };

ADC_ChannelConfTypeDef sConfig = { 0 };

/**
 * @brief Initializes the ADC MSP (MCU Support Package).
 *
 * This function configures the ADC GPIO pins, DMA, and clock settings.
 * It is called by the HAL_ADC_Init function to set up the ADC hardware.
 *
 * @param hadc Pointer to the ADC handle structure.
 */
void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc)
{
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };

    // Enable ADC1 clock
    __HAL_RCC_ADC_CLK_ENABLE();
    // Enable GPIOA clock
    __HAL_RCC_GPIOA_CLK_ENABLE();
    // Enable GPDMA1 clock
    __HAL_RCC_GPDMA1_CLK_ENABLE();

    // Configure GPIO pin for ADC1_IN0 (PA0)
    GPIO_InitStruct.Pin = GPIO_PIN_0; // PA0
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

/**
 * @brief Initializes the ADC1 peripheral.
 *
 * This function configures the ADC1 peripheral with the specified settings.
 * It sets the clock prescaler, resolution, data alignment, and other
 * parameters.
 *
 */
void ADC1_LL_Init(void)
{
    // Initialize the ADC peripheral
    hadc.Instance = ADC1;
    hadc.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc.Init.Resolution = ADC_RESOLUTION_12B;
    hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc.Init.ScanConvMode = DISABLE;
    hadc.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    hadc.Init.LowPowerAutoWait = DISABLE;
    hadc.Init.ContinuousConvMode = DISABLE;
    hadc.Init.NbrOfConversion = 1;
    hadc.Init.DiscontinuousConvMode = DISABLE;
    hadc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc.Init.ExternalTrigConv = ADC_SOFTWARE_START;

    HAL_ADC_Init(&hadc);

    // Configure ADC channel
    sConfig.Channel = ADC_CHANNEL_0; // ADC1_IN0
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_640CYCLES_5;
    HAL_ADC_ConfigChannel(&hadc, &sConfig);
}

/**
 * @brief Deinitializes the ADC
 *
 */
void ADC_LL_DeInit(void)
{
    // Deinitialize the ADC peripheral
    HAL_ADC_DeInit(&hadc);
}

/**
 * @brief Starts the ADC conversion.
 *
 * This function starts the ADC conversion process. It must be called after
 * the ADC has been initialized and configured.
 *
 */
void ADC1_Start(void)
{
    // Start the ADC conversion
    HAL_ADC_Start(&hadc);
}

/**
 * @brief Stops the ADC conversion.
 *
 * This function stops the ongoing ADC conversion process. It can be called
 * to halt conversions before deinitializing the ADC or when no longer needed.
 *
 */
void ADC1_Stop(void)
{
    // Stop the ADC conversion
    HAL_ADC_Stop(&hadc);
}

uint32_t ADC_LL_Read(uint32_t *buffer)
{
    // Start the ADC conversion
    HAL_ADC_Start(&hadc);

    HAL_ADC_PollForConversion(&hadc, HAL_MAX_DELAY);

    // Read the converted value
    *buffer = HAL_ADC_GetValue(&hadc);

    HAL_ADC_Stop(&hadc); // Stop the ADC conversion

    return *buffer; // Return the converted value
}
