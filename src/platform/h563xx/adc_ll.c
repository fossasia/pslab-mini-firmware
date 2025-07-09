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

#include <stdint.h>

#include "stm32h5xx_hal.h"

#include "adc_ll.h"

static ADC_HandleTypeDef g_hadc = { nullptr };

static ADC_ChannelConfTypeDef g_config = { 0 };

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
    (void)hadc; // Suppress unused parameter warning
    GPIO_InitTypeDef gpio_init = { 0 };

    // Enable ADC1 clock
    __HAL_RCC_ADC_CLK_ENABLE();
    // Enable GPIOA clock
    __HAL_RCC_GPIOA_CLK_ENABLE();
    // Enable GPDMA1 clock
    __HAL_RCC_GPDMA1_CLK_ENABLE();

    // Configure GPIO pin for ADC1_IN0 (PA0)
    gpio_init.Pin = GPIO_PIN_0; // PA0
    gpio_init.Mode = GPIO_MODE_ANALOG;
    gpio_init.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio_init);
}

/**
 * @brief Initializes the ADC1 peripheral.
 *
 * This function configures the ADC1 peripheral with the specified settings.
 * It sets the clock prescaler, resolution, data alignment, and other
 * parameters.
 *
 */
void ADC_LL_init(void)
{
    // Initialize the ADC peripheral
    g_hadc.Instance = ADC1;
    g_hadc.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    g_hadc.Init.Resolution = ADC_RESOLUTION_12B;
    g_hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    g_hadc.Init.ScanConvMode = DISABLE;
    g_hadc.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    g_hadc.Init.LowPowerAutoWait = DISABLE;
    g_hadc.Init.ContinuousConvMode = DISABLE;
    g_hadc.Init.NbrOfConversion = 1;
    g_hadc.Init.DiscontinuousConvMode = DISABLE;
    g_hadc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    g_hadc.Init.ExternalTrigConv = ADC_SOFTWARE_START;

    HAL_ADC_Init(&g_hadc);

    // Configure ADC channel
    g_config.Channel = ADC_CHANNEL_0; // ADC1_IN0
    g_config.Rank = ADC_REGULAR_RANK_1;
    g_config.SamplingTime = ADC_SAMPLETIME_640CYCLES_5;
    HAL_ADC_ConfigChannel(&g_hadc, &g_config);
}

/**
 * @brief Deinitializes the ADC
 *
 */
void ADC_LL_deinit(void)
{
    // Deinitialize the ADC peripheral
    HAL_ADC_DeInit(&g_hadc);
}

/**
 * @brief Starts the ADC conversion.
 *
 * This function starts the ADC conversion process. It must be called after
 * the ADC has been initialized and configured.
 *
 */
void ADC_LL_start(void)
{
    // Start the ADC conversion
    HAL_ADC_Start(&g_hadc);
}

/**
 * @brief Stops the ADC conversion.
 *
 * This function stops the ongoing ADC conversion process. It can be called
 * to halt conversions before deinitializing the ADC or when no longer needed.
 *
 */
void ADC_LL_stop(void)
{
    // Stop the ADC conversion
    HAL_ADC_Stop(&g_hadc);
}

uint32_t ADC_LL_read(uint32_t *buffer)
{
    // Start the ADC conversion
    HAL_ADC_Start(&g_hadc);

    HAL_ADC_PollForConversion(&g_hadc, HAL_MAX_DELAY);

    // Read the converted value
    *buffer = HAL_ADC_GetValue(&g_hadc);

    HAL_ADC_Stop(&g_hadc); // Stop the ADC conversion

    return *buffer; // Return the converted value
}
