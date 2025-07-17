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
#include "logging.h"

enum { ADC_IRQ_PRIORITY = 1 }; // ADC interrupt priority

typedef struct {
    ADC_HandleTypeDef *adc_handle; // Pointer to the ADC handle
    ADC_ChannelConfTypeDef adc_config; // ADC channel configuration
    DMA_HandleTypeDef *dma_handle; // Pointer to the DMA handle
    uint32_t *adc_buffer_data; // Pointer to the ADC data buffer
    uint32_t adc_buffer_size; // Size of the ADC data buffer
    ADC_LL_CompleteCallback
        adc_complete_callback; // Callback for ADC completion
    bool initialized; // Flag to indicate if the ADC is initialized
} ADCInstance;

static ADC_HandleTypeDef g_hadc = { nullptr };

static ADC_ChannelConfTypeDef g_config = { 0 };

static DMA_HandleTypeDef g_hdma_adc = { nullptr };

static ADCInstance g_adc_instance = {
    .adc_handle = &g_hadc,
    .dma_handle = &g_hdma_adc,
};

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
    // Enable DMA1 clock
    __HAL_RCC_GPDMA1_CLK_ENABLE();

    // Configure GPIO pin for ADC1_IN0 (PA0)
    gpio_init.Pin = GPIO_PIN_0; // PA0
    gpio_init.Mode = GPIO_MODE_ANALOG;
    gpio_init.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio_init);

    /*DMA for the ADC*/
    g_hdma_adc.Instance = GPDMA1_Channel6; // DMA channel for ADC1
    g_hdma_adc.Init.Request = GPDMA1_REQUEST_ADC1;
    g_hdma_adc.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    g_hdma_adc.Init.Direction = DMA_PERIPH_TO_MEMORY;
    g_hdma_adc.Init.SrcInc = DMA_SINC_FIXED;
    g_hdma_adc.Init.DestInc = DMA_DINC_INCREMENTED;
    g_hdma_adc.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_WORD;
    g_hdma_adc.Init.DestDataWidth = DMA_SRC_DATAWIDTH_WORD;
    g_hdma_adc.Init.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
    g_hdma_adc.Init.SrcBurstLength = 1;
    g_hdma_adc.Init.DestBurstLength = 1;
    g_hdma_adc.Init.TransferAllocatedPort =
        (DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0);
    g_hdma_adc.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    g_hdma_adc.Init.Mode = DMA_NORMAL;
    if (HAL_DMA_Init(&g_hdma_adc) != HAL_OK) {
        THROW(ERROR_HARDWARE_FAULT);
    }
    __HAL_LINKDMA(&g_hadc, DMA_Handle, g_hdma_adc);

    // Enable ADC1 interrupt
    HAL_NVIC_SetPriority(ADC1_IRQn, ADC_IRQ_PRIORITY, 0);
    HAL_NVIC_EnableIRQ(ADC1_IRQn);

    HAL_NVIC_SetPriority(GPDMA1_Channel6_IRQn, ADC_IRQ_PRIORITY, 1);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel6_IRQn);
}

/**
 * @brief Initializes the ADC1 peripheral.
 *
 * This function configures the ADC1 peripheral with the specified settings.
 * It sets the clock prescaler, resolution, data alignment, and other
 * parameters.
 *
 */
void ADC_LL_init(
    uint32_t *adc_buf,
    uint32_t sz,
    ADC_LL_TriggerSource adc_trigger_timer
)
{
    if (adc_buf == nullptr || sz == 0) {
        THROW(ERROR_INVALID_ARGUMENT);
        return;
    }

    if (g_adc_instance.initialized) {
        THROW(ERROR_RESOURCE_BUSY);
        return;
    }

    // Initialize the ADC handle
    ADCInstance *instance = &g_adc_instance;

    // Initialize the ADC peripheral
    instance->adc_handle->Instance = ADC1;
    instance->adc_handle->Init.ClockPrescaler =
        ADC_CLOCK_SYNC_PCLK_DIV1; // ADC clock and prescaler
    instance->adc_handle->Init.Resolution = ADC_RESOLUTION_12B;
    instance->adc_handle->Init.DataAlign = ADC_DATAALIGN_RIGHT;
    instance->adc_handle->Init.ScanConvMode = DISABLE;
    instance->adc_handle->Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    instance->adc_handle->Init.LowPowerAutoWait = DISABLE;
    instance->adc_handle->Init.ContinuousConvMode = DISABLE;
    instance->adc_handle->Init.NbrOfConversion = 1;
    instance->adc_handle->Init.DiscontinuousConvMode = DISABLE;
    if (adc_trigger_timer == ADC_TRIGGER_TIMER6) {
        instance->adc_handle->Init.ExternalTrigConv = ADC_EXTERNALTRIG_T6_TRGO;
    } else {
        THROW(ERROR_INVALID_ARGUMENT);
        return;
    }
    instance->adc_handle->Init.ExternalTrigConvEdge =
        ADC_EXTERNALTRIGCONVEDGE_RISING;
    instance->adc_handle->Init.SamplingMode = ADC_SAMPLING_MODE_NORMAL;
    instance->adc_handle->Init.DMAContinuousRequests = DISABLE;
    instance->adc_handle->Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
    instance->adc_handle->Init.OversamplingMode = DISABLE;

    if (HAL_ADC_Init(&g_hadc) != HAL_OK) {
        THROW(ERROR_HARDWARE_FAULT);
    }

    // Configure the ADC channel
    instance->adc_config = g_config;
    g_config.Channel = ADC_CHANNEL_0; // ADC1_IN0
    g_config.Rank = ADC_REGULAR_RANK_1;
    g_config.SamplingTime = ADC_SAMPLETIME_12CYCLES_5;
    g_config.SingleDiff = ADC_SINGLE_ENDED; // Single-ended input
    g_config.OffsetNumber = ADC_OFFSET_NONE;
    g_config.Offset = 0;
    if (HAL_ADC_ConfigChannel(instance->adc_handle, &g_config) != HAL_OK) {
        THROW(ERROR_HARDWARE_FAULT);
    }

    // Calibration with error handling
    if (HAL_ADCEx_Calibration_Start(&g_hadc, ADC_SINGLE_ENDED) != HAL_OK) {
        THROW(ERROR_HARDWARE_FAULT);
    } // Calibration

    // Set the ADC buffer and size
    instance->adc_buffer_data = adc_buf;
    instance->adc_buffer_size = sz;
    instance->initialized = true;
}

/**
 * @brief Deinitializes the ADC
 *
 */
void ADC_LL_deinit(void)
{
    if (!g_adc_instance.initialized) {
        THROW(ERROR_RESOURCE_UNAVAILABLE);
        return;
    }

    ADCInstance *instance = &g_adc_instance;
    // Stop the ADC conversion;
    HAL_NVIC_DisableIRQ(ADC1_IRQn); // Disable ADC1 interrupt

    // Deinitialize the ADC peripheral
    if (HAL_ADC_DeInit(instance->adc_handle) != HAL_OK) {
        THROW(ERROR_HARDWARE_FAULT);
    }

    instance->adc_buffer_data = nullptr;
    instance->adc_buffer_size = 0;
    instance->adc_complete_callback = nullptr; // Clear the callback
    instance->initialized = false;
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
    if (HAL_ADC_Start_DMA(
            &g_hadc,
            g_adc_instance.adc_buffer_data,
            g_adc_instance.adc_buffer_size
        ) != HAL_OK) {
        THROW(ERROR_HARDWARE_FAULT);
    } // Start ADC in DMA mode

    __HAL_ADC_CLEAR_FLAG(&g_hadc, ADC_FLAG_OVR); // Clear any previous flags
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
    if (HAL_ADC_Stop_DMA(&g_hadc) != HAL_OK) {
        THROW(ERROR_HARDWARE_FAULT);
    }
}

/**
 * @brief Sets the ADC complete callback.
 *
 * This function sets the user-defined callback that will be called when the
 * ADC conversion is complete.
 *
 * @param callback Pointer to the callback function.
 */
void ADC_LL_set_complete_callback(ADC_LL_CompleteCallback callback)
{
    g_adc_instance.adc_complete_callback =
        callback; // Set the user-defined callback
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    (void)hadc; // Suppress unused parameter warning

    if (g_adc_instance.adc_complete_callback != nullptr) {
        g_adc_instance.adc_complete_callback(
        ); // Call the user-defined callback with the ADC value
    }
}

void ADC1_IRQHandler(void) { HAL_ADC_IRQHandler(&g_hadc); }

void GPDMA1_Channel6_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&g_hdma_adc); // Handle DMA interrupts
}