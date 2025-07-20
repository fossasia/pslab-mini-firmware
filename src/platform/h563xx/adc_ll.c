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

bool g_adc1_initialized = false; // Flag to indicate if ADC1 is initialized
bool g_adc2_initialized = false; // Flag to indicate if ADC2 is initialized
bool g_adc_multimode =
    true; // Flag to indicate if ADC1 and ADC2 are in multimode

typedef struct {
    ADC_HandleTypeDef *adc_handle; // Pointer to the ADC handle
    ADC_ChannelConfTypeDef adc_config; // ADC channel configuration
    DMA_HandleTypeDef *dma_handle; // Pointer to the DMA handle
    uint16_t *adc_buffer_data; // Pointer to the ADC data buffer
    uint32_t adc_buffer_size; // Size of the ADC data buffer
    ADC_LL_CompleteCallback
        adc_complete_callback; // Callback for ADC completion
} ADCInstance;

static ADC_HandleTypeDef g_hadc1 = { nullptr };
static ADC_HandleTypeDef g_hadc2 = { nullptr };

static ADC_ChannelConfTypeDef g_config1 = { 0 };
static ADC_ChannelConfTypeDef g_config2 = { 0 };

static DMA_HandleTypeDef g_hdma_adc1 = { nullptr };
static DMA_HandleTypeDef g_hdma_adc2 = { nullptr };

static ADCInstance g_adc_instances[ADC_COUNT] = {
    [ADC_1] = {
        .adc_handle = &g_hadc1,
        .dma_handle = &g_hdma_adc1,
    },
    [ADC_2] = {
        .adc_handle = &g_hadc2,
        .dma_handle = &g_hdma_adc2,
    },
    [ADC_1_2] = {
        .adc_handle = &g_hadc1, // Use ADC1 for dual mode
        .dma_handle = &g_hdma_adc1,
    }
};

static ADC_Num get_adc_num_from_handle(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1 || !g_adc_multimode) {
        return ADC_1;
    }
    if (hadc->Instance == ADC2 || !g_adc_multimode) {
        return ADC_2;
    }
    if (hadc->Instance == ADC1 || g_adc_multimode) {
        return ADC_1_2;
    }
    return ADC_COUNT; // Invalid ADC instance
}

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
    if (hadc->Instance == ADC1) {
        // Enable ADC1 clock
        __HAL_RCC_ADC_CLK_ENABLE();
        // Enable GPIOA clock
        __HAL_RCC_GPIOA_CLK_ENABLE();
        // Enable DMA1 clock
        __HAL_RCC_GPDMA1_CLK_ENABLE();
        if (g_adc_multimode) {
            // Configure GPIO pin for ADC1_IN14 (PA14)
            gpio_init.Pin = GPIO_PIN_14; // PA14
        } else {
            // Configure GPIO pin for ADC1_IN0 (PA0)
            gpio_init.Pin = GPIO_PIN_0; // PA0
        }
        gpio_init.Mode = GPIO_MODE_ANALOG;
        gpio_init.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &gpio_init);

        /*DMA for the ADC*/
        g_hdma_adc1.Instance = GPDMA1_Channel6; // DMA channel for ADC1
        g_hdma_adc1.Init.Request = GPDMA1_REQUEST_ADC1;
        g_hdma_adc1.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
        g_hdma_adc1.Init.Direction = DMA_PERIPH_TO_MEMORY;
        g_hdma_adc1.Init.SrcInc = DMA_SINC_FIXED;
        g_hdma_adc1.Init.DestInc = DMA_DINC_INCREMENTED;
        g_hdma_adc1.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_HALFWORD;
        g_hdma_adc1.Init.DestDataWidth = DMA_DEST_DATAWIDTH_HALFWORD;
        g_hdma_adc1.Init.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
        g_hdma_adc1.Init.SrcBurstLength = 1;
        g_hdma_adc1.Init.DestBurstLength = 1;
        g_hdma_adc1.Init.TransferAllocatedPort =
            (DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0);
        g_hdma_adc1.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
        g_hdma_adc1.Init.Mode = DMA_NORMAL;
        if (HAL_DMA_Init(&g_hdma_adc1) != HAL_OK) {
            THROW(ERROR_HARDWARE_FAULT);
        }
        __HAL_LINKDMA(&g_hadc1, DMA_Handle, g_hdma_adc1);

        // Enable ADC1 interrupt
        HAL_NVIC_SetPriority(ADC1_IRQn, ADC_IRQ_PRIORITY, 0);
        HAL_NVIC_EnableIRQ(ADC1_IRQn);

        HAL_NVIC_SetPriority(GPDMA1_Channel6_IRQn, ADC_IRQ_PRIORITY, 1);
        HAL_NVIC_EnableIRQ(GPDMA1_Channel6_IRQn);
    } else if (hadc->Instance == ADC2 || !g_adc_multimode) {
        // Enable ADC2 clock
        __HAL_RCC_ADC_CLK_ENABLE();
        // Enable GPIOA clock
        __HAL_RCC_GPIOA_CLK_ENABLE();
        // Enable DMA1 clock
        __HAL_RCC_GPDMA1_CLK_ENABLE();
        // Configure GPIO pin for ADC2_IN1 (PA1)
        GPIO_InitTypeDef gpio_init = { 0 };
        gpio_init.Pin = GPIO_PIN_1; // PA1
        gpio_init.Mode = GPIO_MODE_ANALOG;
        gpio_init.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &gpio_init);
        /*DMA for the ADC*/
        g_hdma_adc2.Instance = GPDMA1_Channel7; // DMA channel for ADC2
        g_hdma_adc2.Init.Request = GPDMA1_REQUEST_ADC2;
        g_hdma_adc2.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
        g_hdma_adc2.Init.Direction = DMA_PERIPH_TO_MEMORY;
        g_hdma_adc2.Init.SrcInc = DMA_SINC_FIXED;
        g_hdma_adc2.Init.DestInc = DMA_DINC_INCREMENTED;
        g_hdma_adc2.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_HALFWORD;
        g_hdma_adc2.Init.DestDataWidth = DMA_DEST_DATAWIDTH_HALFWORD;
        g_hdma_adc2.Init.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
        g_hdma_adc2.Init.SrcBurstLength = 1;
        g_hdma_adc2.Init.DestBurstLength = 1;
        g_hdma_adc2.Init.TransferAllocatedPort =
            (DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0);
        g_hdma_adc2.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
        g_hdma_adc2.Init.Mode = DMA_NORMAL;
        if (HAL_DMA_Init(&g_hdma_adc2) != HAL_OK) {
            THROW(ERROR_HARDWARE_FAULT);
        }

        __HAL_LINKDMA(&g_hadc2, DMA_Handle, g_hdma_adc2);

        // Enable ADC2 interrupt
        HAL_NVIC_SetPriority(ADC2_IRQn, ADC_IRQ_PRIORITY, 0);
        HAL_NVIC_EnableIRQ(ADC2_IRQn);

        HAL_NVIC_SetPriority(GPDMA1_Channel7_IRQn, ADC_IRQ_PRIORITY, 1);
        HAL_NVIC_EnableIRQ(GPDMA1_Channel7_IRQn);
    }

    else if (hadc->Instance == ADC2 || g_adc_multimode) {
        // Configure GPIO pin for ADC2_IN7 (PA7)
        GPIO_InitTypeDef gpio_init = { 0 };
        gpio_init.Pin = GPIO_PIN_7; // PA7
        gpio_init.Mode = GPIO_MODE_ANALOG;
        gpio_init.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &gpio_init);
        // Enable ADC2 interrupt
        HAL_NVIC_SetPriority(ADC2_IRQn, ADC_IRQ_PRIORITY, 0);
        HAL_NVIC_EnableIRQ(ADC2_IRQn);
    }
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
    ADC_Num adc_num,
    uint16_t *adc_buf,
    uint32_t sz,
    ADC_LL_TriggerSource adc_trigger_timer
)
{
    if (adc_buf == nullptr || sz == 0) {
        THROW(ERROR_INVALID_ARGUMENT);
        return;
    }

    // Initialize the ADC handle
    ADCInstance *instance = &g_adc_instances[adc_num];

    if (adc_num == ADC_1) {
        if (g_adc1_initialized) {
            THROW(ERROR_RESOURCE_BUSY);
            return;
        }

        instance->adc_handle->Instance = ADC1;
        // Initialize the ADC peripheral
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
            instance->adc_handle->Init.ExternalTrigConv =
                ADC_EXTERNALTRIG_T6_TRGO;
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

        if (HAL_ADC_Init(instance->adc_handle) != HAL_OK) {
            THROW(ERROR_HARDWARE_FAULT);
        }

        // Configure the ADC channel
        instance->adc_config = g_config1;
        if (!g_adc_multimode) {
            g_config1.Channel = ADC_CHANNEL_0; // ADC1_IN0
        } else if (g_adc_multimode) {
            g_config1.Channel = ADC_CHANNEL_3; // ADC1_INP3
        }
        g_config1.Rank = ADC_REGULAR_RANK_1;
        g_config1.SamplingTime = ADC_SAMPLETIME_12CYCLES_5;
        g_config1.SingleDiff = ADC_SINGLE_ENDED; // Single-ended input
        g_config1.OffsetNumber = ADC_OFFSET_NONE;
        g_config1.Offset = 0;
        if (HAL_ADC_ConfigChannel(instance->adc_handle, &g_config1) != HAL_OK) {
            THROW(ERROR_HARDWARE_FAULT);
        }

        // Calibration with error handling
        if (HAL_ADCEx_Calibration_Start(
                instance->adc_handle, ADC_SINGLE_ENDED
            ) != HAL_OK) {
            THROW(ERROR_HARDWARE_FAULT);
        } // Calibration
        if (!g_adc_multimode) {
            instance->adc_buffer_data = adc_buf;
            instance->adc_buffer_size = sz;
            g_adc1_initialized = true; // Set the ADC1 initialized flag
        }
    }

    else if (adc_num == ADC_2) {
        if (g_adc2_initialized) {
            THROW(ERROR_RESOURCE_BUSY);
            return;
        }
        instance->adc_handle->Instance = ADC2;
        // Initialize the ADC peripheral
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
            instance->adc_handle->Init.ExternalTrigConv =
                ADC_EXTERNALTRIG_T6_TRGO;
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

        if (HAL_ADC_Init(instance->adc_handle) != HAL_OK) {
            THROW(ERROR_HARDWARE_FAULT);
        }

        // Configure the ADC channel
        instance->adc_config = g_config2;
        g_config2.Channel = ADC_CHANNEL_1; // ADC2_IN1
        g_config2.Rank = ADC_REGULAR_RANK_1;
        g_config2.SamplingTime = ADC_SAMPLETIME_12CYCLES_5;
        g_config2.SingleDiff = ADC_SINGLE_ENDED; // Single-ended input
        g_config2.OffsetNumber = ADC_OFFSET_NONE;
        g_config2.Offset = 0;
        if (HAL_ADC_ConfigChannel(instance->adc_handle, &g_config2) != HAL_OK) {
            THROW(ERROR_HARDWARE_FAULT);
        }

        // Calibration with error handling
        if (HAL_ADCEx_Calibration_Start(
                instance->adc_handle, ADC_SINGLE_ENDED
            ) != HAL_OK) {
            THROW(ERROR_HARDWARE_FAULT);
        } // Calibration
        if (!g_adc_multimode) {
            instance->adc_buffer_data = adc_buf;
            instance->adc_buffer_size = sz;
            g_adc2_initialized = true; // Set the ADC1 initialized flag
        }
    }

    else if (adc_num == ADC_1_2) {
        if (g_adc1_initialized || g_adc2_initialized) {
            THROW(ERROR_RESOURCE_BUSY);
            return;
        }

        g_adc_multimode = true; // Set multimode flag

        ADC_LL_init(ADC_1, adc_buf, sz, adc_trigger_timer);

        instance->adc_handle->Instance = ADC2;
        // Initialize the ADC peripheral
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
            instance->adc_handle->Init.ExternalTrigConv =
                ADC_EXTERNALTRIG_T6_TRGO;
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

        if (HAL_ADC_Init(instance->adc_handle) != HAL_OK) {
            THROW(ERROR_HARDWARE_FAULT);
        }

        // Configure the ADC channel
        instance->adc_config = g_config2;
        g_config2.Channel = ADC_CHANNEL_7; // ADC2_IN7
        g_config2.Rank = ADC_REGULAR_RANK_1;
        g_config2.SamplingTime = ADC_SAMPLETIME_12CYCLES_5;
        g_config2.SingleDiff = ADC_SINGLE_ENDED; // Single-ended input
        g_config2.OffsetNumber = ADC_OFFSET_NONE;
        g_config2.Offset = 0;
        if (HAL_ADC_ConfigChannel(instance->adc_handle, &g_config2) != HAL_OK) {
            THROW(ERROR_HARDWARE_FAULT);
        }

        // Calibration with error handling
        if (HAL_ADCEx_Calibration_Start(
                instance->adc_handle, ADC_SINGLE_ENDED
            ) != HAL_OK) {
            THROW(ERROR_HARDWARE_FAULT);
        } // Calibration

        ADC_MultiModeTypeDef multimode_config = { 0 };
        multimode_config.Mode = ADC_DUALMODE_INTERL;
        multimode_config.DMAAccessMode = ADC_DMAACCESSMODE_12_10_BITS;
        multimode_config.TwoSamplingDelay = ADC_TWOSAMPLINGDELAY_12CYCLES;

        if (HAL_ADCEx_MultiModeConfigChannel(&g_hadc1, &multimode_config) !=
            HAL_OK) {
            THROW(ERROR_HARDWARE_FAULT);
        } // Configure multimode
        instance->adc_handle =
            &g_hadc1; // Use ADC1 handle for multimode(adding this again here to
                      // ensure that when this pointer is called later for
                      // example in ADC_LL_start, it points to ADC1 handle)
        instance->adc_buffer_data = adc_buf;
        instance->adc_buffer_size = sz;
        g_adc1_initialized = true; // Set the ADC1 initialized flag
        g_adc2_initialized = true; // Set the ADC2 initialized flag
    }

    else {
        THROW(ERROR_INVALID_ARGUMENT);
        return;
    }
}

/**
 * @brief Deinitializes the ADC
 *
 */
void ADC_LL_deinit(ADC_Num adc_num)
{
    if (adc_num >= ADC_COUNT) {
        THROW(ERROR_INVALID_ARGUMENT);
        return;
    }

    ADCInstance *instance = &g_adc_instances[adc_num];
    // Stop the ADC conversion;
    if (adc_num == ADC_1) {
        HAL_NVIC_DisableIRQ(ADC1_IRQn); // Disable ADC1 interrupt
        g_adc1_initialized = false; // Clear the ADC1 initialized flag
    } else if (adc_num == ADC_2) {
        HAL_NVIC_DisableIRQ(ADC2_IRQn); // Disable ADC2 interrupt
        g_adc2_initialized = false; // Clear the ADC2 initialized flag
    } else if (adc_num == ADC_1_2) {
        HAL_NVIC_DisableIRQ(ADC1_IRQn); // Disable ADC1 interrupt
        HAL_NVIC_DisableIRQ(ADC2_IRQn); // Disable ADC2 interrupt
        g_adc1_initialized = false; // Clear the ADC1 initialized flag
        g_adc2_initialized = false; // Clear the ADC2 initialized flag
    }

    // Deinitialize the ADC peripheral
    if (HAL_ADC_DeInit(instance->adc_handle) != HAL_OK) {
        THROW(ERROR_HARDWARE_FAULT);
    }

    instance->adc_buffer_data = nullptr;
    instance->adc_buffer_size = 0;
    instance->adc_complete_callback = nullptr; // Clear the callback
}

/**
 * @brief Starts the ADC conversion.
 *
 * This function starts the ADC conversion process. It must be called after
 * the ADC has been initialized and configured.
 *
 */
void ADC_LL_start(ADC_Num adc_num)
{
    ADCInstance *instance = &g_adc_instances[adc_num];
    __HAL_ADC_CLEAR_FLAG(
        instance->adc_handle, ADC_FLAG_OVR
    ); // Clear any previous flags

    if (HAL_ADC_Start_DMA(
            instance->adc_handle,
            (uint32_t *)instance->adc_buffer_data,
            instance->adc_buffer_size
        ) != HAL_OK) {
        THROW(ERROR_HARDWARE_FAULT);
    } // Start ADC in DMA mode
}

/**
 * @brief Stops the ADC conversion.
 *
 * This function stops the ongoing ADC conversion process. It can be called
 * to halt conversions before deinitializing the ADC or when no longer needed.
 *
 */
void ADC_LL_stop(ADC_Num adc_num)
{
    if (adc_num >= ADC_COUNT) {
        THROW(ERROR_INVALID_ARGUMENT);
        return;
    }

    ADCInstance *instance = &g_adc_instances[adc_num];
    if (instance->adc_handle == nullptr) {
        THROW(ERROR_DEVICE_NOT_READY);
        return;
    }

    // Stop the ADC conversion
    if (HAL_ADC_Stop_DMA(instance->adc_handle) != HAL_OK) {
        THROW(ERROR_HARDWARE_FAULT);
    } // Stop ADC in DMA mode

    // Clear any pending flags
    __HAL_ADC_CLEAR_FLAG(instance->adc_handle, ADC_FLAG_EOC);
    __HAL_ADC_CLEAR_FLAG(instance->adc_handle, ADC_FLAG_OVR);
}
/**
 * @brief Sets the ADC complete callback.
 *
 * This function sets the user-defined callback that will be called when the
 * ADC conversion is complete.
 *
 * @param callback Pointer to the callback function.
 */
void ADC_LL_set_complete_callback(
    ADC_Num adc_num,
    ADC_LL_CompleteCallback callback
)
{
    g_adc_instances[adc_num].adc_complete_callback =
        callback; // Set the user-defined callback
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    ADC_Num adc_num = get_adc_num_from_handle(hadc);
    if (g_adc_instances[adc_num].adc_complete_callback != nullptr) {
        g_adc_instances[adc_num].adc_complete_callback(adc_num
        ); // Call the user-defined callback with the ADC value
    }
}

void ADC1_IRQHandler(void) { HAL_ADC_IRQHandler(&g_hadc1); }

void ADC2_IRQHandler(void) { HAL_ADC_IRQHandler(&g_hadc2); }

void GPDMA1_Channel6_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&g_hdma_adc1); // Handle DMA interrupts
}

void GPDMA1_Channel7_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&g_hdma_adc2); // Handle DMA interrupts
}