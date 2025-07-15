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
#include "error.h"
#include "logging_ll.h"

enum { ADC_IRQ_PRIORITY = 1 }; // ADC interrupt priority

static ADC_HandleTypeDef g_hadc = { nullptr };

static ADC_ChannelConfTypeDef g_config = { 0 };

static ADC_LL_CompleteCallback volatile g_adc_complete_callback;

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

    // Configure GPIO pin for ADC1_IN0 (PA0)
    gpio_init.Pin = GPIO_PIN_0; // PA0
    gpio_init.Mode = GPIO_MODE_ANALOG;
    gpio_init.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio_init);

    // Enable ADC1 interrupt
    HAL_NVIC_SetPriority(ADC1_IRQn, ADC_IRQ_PRIORITY, 0);
    HAL_NVIC_EnableIRQ(ADC1_IRQn);
}

/**
 * @brief Initializes the ADC1 peripheral.
 *
 * This function configures the ADC1 peripheral with the specified settings.
 * It sets the clock prescaler, resolution, data alignment, and other
 * parameters.
 *
 */
void ADC_LL_init(ADC_LL_TriggerSource adc_trigger_timer)
{

    // Initialize the ADC peripheral
    g_hadc.Instance = ADC1;
    g_hadc.Init.ClockPrescaler =
        ADC_CLOCK_SYNC_PCLK_DIV1; // ADC clock and prescaler
    g_hadc.Init.Resolution = ADC_RESOLUTION_12B;
    g_hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    g_hadc.Init.ScanConvMode = DISABLE;
    g_hadc.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    g_hadc.Init.LowPowerAutoWait = DISABLE;
    g_hadc.Init.ContinuousConvMode = DISABLE;
    g_hadc.Init.NbrOfConversion = 1;
    g_hadc.Init.DiscontinuousConvMode = DISABLE;
    if (adc_trigger_timer == ADC_TRIGGER_TIMER6) {
        g_hadc.Init.ExternalTrigConv = ADC_EXTERNALTRIG_T6_TRGO;
    } else {
        THROW(ERROR_INVALID_ARGUMENT);
        return;
    }
    g_hadc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
    g_hadc.Init.SamplingMode = ADC_SAMPLING_MODE_NORMAL;
    g_hadc.Init.DMAContinuousRequests = DISABLE;
    g_hadc.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
    g_hadc.Init.OversamplingMode = DISABLE;

    if (HAL_ADC_Init(&g_hadc) != HAL_OK) {
        THROW(ERROR_HARDWARE_FAULT);
    }

    // Configure ADC channel
    g_config.Channel = ADC_CHANNEL_0; // ADC1_IN0
    g_config.Rank = ADC_REGULAR_RANK_1;
    g_config.SamplingTime = ADC_SAMPLETIME_12CYCLES_5;
    g_config.SingleDiff = ADC_SINGLE_ENDED; // Single-ended input
    g_config.OffsetNumber = ADC_OFFSET_NONE;
    g_config.Offset = 0;
    if (HAL_ADC_ConfigChannel(&g_hadc, &g_config) != HAL_OK) {
        THROW(ERROR_HARDWARE_FAULT);
    }

    // Calibration with error handling
    if (HAL_ADCEx_Calibration_Start(&g_hadc, ADC_SINGLE_ENDED) != HAL_OK) {
        THROW(ERROR_HARDWARE_FAULT);
    } // Calibration
}

/**
 * @brief Deinitializes the ADC
 *
 */
void ADC_LL_deinit(void)
{
    // Deinitialize the ADC peripheral
    if (HAL_ADC_DeInit(&g_hadc) != HAL_OK) {
        THROW(ERROR_HARDWARE_FAULT);
    }
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
    if (HAL_ADC_Start_IT(&g_hadc) != HAL_OK) {
        THROW(ERROR_HARDWARE_FAULT);
    }
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
    if (HAL_ADC_Stop_IT(&g_hadc) != HAL_OK) {
        THROW(ERROR_HARDWARE_FAULT);
    }
}

void ADC_LL_set_complete_callback(ADC_LL_CompleteCallback callback)
{
    g_adc_complete_callback = callback; // Set the user-defined callback
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    (void)hadc; // Suppress unused parameter warning
    uint32_t value = HAL_ADC_GetValue(&g_hadc);
    if (g_adc_complete_callback != nullptr) {
        g_adc_complete_callback(value
        ); // Call the user-defined callback with the ADC value
    }
}

void ADC1_IRQHandler(void) { HAL_ADC_IRQHandler(&g_hadc); }
