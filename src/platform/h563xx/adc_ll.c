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
#include "platform.h"

enum { ADC_IRQ_PRIORITY = 1 }; // ADC interrupt priority

bool g_adc1_initialized = false; // Flag to indicate if ADC1 is initialized
bool g_adc2_initialized = false; // Flag to indicate if ADC2 is initialized
bool g_adc_multimode =
    false; // Flag to indicate if ADC1 and ADC2 are in multimode

typedef struct {
    ADC_HandleTypeDef *adc_handle; // Pointer to the ADC handle
    ADC_ChannelConfTypeDef adc_config; // ADC channel configuration
    DMA_HandleTypeDef *dma_handle; // Pointer to the DMA handle
    uint32_t *adc_buffer_data; // Pointer to the ADC data buffer
    uint32_t adc_buffer_size; // Size of the ADC data buffer
    ADC_LL_CompleteCallback
        adc_complete_callback; // Callback for ADC completion
    ADC_LL_Channel current_channel; // Current ADC channel
    uint32_t oversampling_ratio; // Oversampling ratio
    bool initialized; // Flag to indicate if the ADC is initialized
} ADCInstance;

static ADC_HandleTypeDef g_hadc1 = { nullptr };
static ADC_HandleTypeDef g_hadc2 = { nullptr };

static ADC_ChannelConfTypeDef g_config1 = { 0 };
static ADC_ChannelConfTypeDef g_config2 = { 0 };

static DMA_HandleTypeDef g_hdma_adc1 = { nullptr };
static DMA_HandleTypeDef g_hdma_adc2 = { nullptr };

typedef struct {
    GPIO_TypeDef *gpio_port;
    uint16_t gpio_pin;
} ADC_LL_PinConfig;

static ADCInstance g_adc_instance = {
    .adc_handle = &g_hadc,
    .dma_handle = &g_hdma_adc,
    .current_channel = ADC_LL_CHANNEL_0,
    .oversampling_ratio = 1,
};

/**
 * @brief Gets the GPIO pin configuration for a given ADC channel.
 *
 * This function returns the GPIO port and pin configuration for the
 * specified ADC channel on STM32H563. This is an internal function
 * used by the implementation only.
 *
 * @param channel ADC channel.
 * @return GPIO pin configuration structure.
 */
static ADC_LL_PinConfig get_pin_config(ADC_LL_Channel channel)
{
    ADC_LL_PinConfig config = { 0 };

    switch (channel) {
    case ADC_LL_CHANNEL_0:
        config.gpio_port = GPIOA;
        config.gpio_pin = GPIO_PIN_0;
        break;
    case ADC_LL_CHANNEL_1:
        config.gpio_port = GPIOA;
        config.gpio_pin = GPIO_PIN_1;
        break;
    case ADC_LL_CHANNEL_2:
        config.gpio_port = GPIOA;
        config.gpio_pin = GPIO_PIN_2;
        break;
    case ADC_LL_CHANNEL_3:
        config.gpio_port = GPIOA;
        config.gpio_pin = GPIO_PIN_3;
        break;
    case ADC_LL_CHANNEL_4:
        config.gpio_port = GPIOA;
        config.gpio_pin = GPIO_PIN_4;
        break;
    case ADC_LL_CHANNEL_5:
        config.gpio_port = GPIOA;
        config.gpio_pin = GPIO_PIN_5;
        break;
    case ADC_LL_CHANNEL_6:
        config.gpio_port = GPIOA;
        config.gpio_pin = GPIO_PIN_6;
        break;
    case ADC_LL_CHANNEL_7:
        config.gpio_port = GPIOA;
        config.gpio_pin = GPIO_PIN_7;
        break;
    case ADC_LL_CHANNEL_8:
        config.gpio_port = GPIOB;
        config.gpio_pin = GPIO_PIN_0;
        break;
    case ADC_LL_CHANNEL_9:
        config.gpio_port = GPIOB;
        config.gpio_pin = GPIO_PIN_1;
        break;
    case ADC_LL_CHANNEL_10:
        config.gpio_port = GPIOC;
        config.gpio_pin = GPIO_PIN_0;
        break;
    case ADC_LL_CHANNEL_11:
        config.gpio_port = GPIOC;
        config.gpio_pin = GPIO_PIN_1;
        break;
    case ADC_LL_CHANNEL_12:
        config.gpio_port = GPIOC;
        config.gpio_pin = GPIO_PIN_2;
        break;
    case ADC_LL_CHANNEL_13:
        config.gpio_port = GPIOC;
        config.gpio_pin = GPIO_PIN_3;
        break;
    case ADC_LL_CHANNEL_14:
        config.gpio_port = GPIOC;
        config.gpio_pin = GPIO_PIN_4;
        break;
    case ADC_LL_CHANNEL_15:
        config.gpio_port = GPIOC;
        config.gpio_pin = GPIO_PIN_5;
        break;
    default:
        // Default to channel 0
        config.gpio_port = GPIOA;
        config.gpio_pin = GPIO_PIN_0;
        break;
    }

    return config;
}

/**
 * @brief Converts ADC_LL_Channel to HAL ADC channel constant.
 *
 * @param channel ADC_LL_Channel enum value.
 * @return Corresponding HAL ADC channel constant.
 */
static uint32_t get_hal_adc_channel(ADC_LL_Channel channel)
{
    switch (channel) {
    case ADC_LL_CHANNEL_0:
        return ADC_CHANNEL_0;
    case ADC_LL_CHANNEL_1:
        return ADC_CHANNEL_1;
    case ADC_LL_CHANNEL_2:
        return ADC_CHANNEL_2;
    case ADC_LL_CHANNEL_3:
        return ADC_CHANNEL_3;
    case ADC_LL_CHANNEL_4:
        return ADC_CHANNEL_4;
    case ADC_LL_CHANNEL_5:
        return ADC_CHANNEL_5;
    case ADC_LL_CHANNEL_6:
        return ADC_CHANNEL_6;
    case ADC_LL_CHANNEL_7:
        return ADC_CHANNEL_7;
    case ADC_LL_CHANNEL_8:
        return ADC_CHANNEL_8;
    case ADC_LL_CHANNEL_9:
        return ADC_CHANNEL_9;
    case ADC_LL_CHANNEL_10:
        return ADC_CHANNEL_10;
    case ADC_LL_CHANNEL_11:
        return ADC_CHANNEL_11;
    case ADC_LL_CHANNEL_12:
        return ADC_CHANNEL_12;
    case ADC_LL_CHANNEL_13:
        return ADC_CHANNEL_13;
    case ADC_LL_CHANNEL_14:
        return ADC_CHANNEL_14;
    case ADC_LL_CHANNEL_15:
        return ADC_CHANNEL_15;
    default:
        return ADC_CHANNEL_0;
    }
}

/**
 * @brief Converts ADC_LL_TriggerSource to HAL trigger constant.
 *
 * @param trigger ADC_LL_TriggerSource enum value.
 * @return Corresponding HAL trigger constant.
 */
static uint32_t get_hal_trigger_source(ADC_LL_TriggerSource trigger)
{
    switch (trigger) {
    case ADC_TRIGGER_TIMER1:
        return ADC_EXTERNALTRIG_T1_TRGO;
    case ADC_TRIGGER_TIMER1_TRGO2:
        return ADC_EXTERNALTRIG_T1_TRGO2;
    case ADC_TRIGGER_TIMER2:
        return ADC_EXTERNALTRIG_T2_TRGO;
    case ADC_TRIGGER_TIMER3:
        return ADC_EXTERNALTRIG_T3_TRGO;
    case ADC_TRIGGER_TIMER4:
        return ADC_EXTERNALTRIG_T4_TRGO;
    case ADC_TRIGGER_TIMER6:
        return ADC_EXTERNALTRIG_T6_TRGO;
    case ADC_TRIGGER_TIMER8:
        return ADC_EXTERNALTRIG_T8_TRGO;
    case ADC_TRIGGER_TIMER8_TRGO2:
        return ADC_EXTERNALTRIG_T8_TRGO2;
    case ADC_TRIGGER_TIMER15:
        return ADC_EXTERNALTRIG_T15_TRGO;
    default:
        return ADC_EXTERNALTRIG_T6_TRGO;
    }
}

/**
 * @brief Initializes the ADC MSP (MCU Support Package).
 *
 * This function configures the ADC GPIO pins, DMA, and clock settings.
 * It is called by the HAL_ADC_Init function to set up the ADC hardware.
 *
 * @param hadc Pointer to the ADC handle structure.
 */ // NOLINTNEXTLINE(readability-function-cognitive-complexity)
void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc)
{
    (void)hadc; // Suppress unused parameter warning
    GPIO_InitTypeDef gpio_init = { 0 };
    ADC_LL_PinConfig pin_config;

    // Enable ADC1 clock
    __HAL_RCC_ADC_CLK_ENABLE();

    // Enable GPIO clocks based on current channel
    pin_config = get_pin_config(g_adc_instance.current_channel);

    if (pin_config.gpio_port == GPIOA) {
        __HAL_RCC_GPIOA_CLK_ENABLE();
    } else if (pin_config.gpio_port == GPIOB) {
        __HAL_RCC_GPIOB_CLK_ENABLE();
    } else if (pin_config.gpio_port == GPIOC) {
        __HAL_RCC_GPIOC_CLK_ENABLE();
    }

    // Enable DMA1 clock
    __HAL_RCC_GPDMA1_CLK_ENABLE();

    // Configure GPIO pin for ADC input
    gpio_init.Pin = pin_config.gpio_pin;
    gpio_init.Mode = GPIO_MODE_ANALOG;
    gpio_init.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(pin_config.gpio_port, &gpio_init);

    // Configure DMA
    g_hdma_adc.Instance = GPDMA1_Channel6; // DMA channel for ADC1
    g_hdma_adc.Init.Request = GPDMA1_REQUEST_ADC1;
    g_hdma_adc.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    g_hdma_adc.Init.Direction = DMA_PERIPH_TO_MEMORY;
    g_hdma_adc.Init.SrcInc = DMA_SINC_FIXED;
    g_hdma_adc.Init.DestInc = DMA_DINC_INCREMENTED;
    g_hdma_adc.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_HALFWORD;
    g_hdma_adc.Init.DestDataWidth = DMA_DEST_DATAWIDTH_HALFWORD;
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

    // Enable DMA interrupt
    HAL_NVIC_SetPriority(GPDMA1_Channel6_IRQn, ADC_IRQ_PRIORITY, 1);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel6_IRQn);

    // Enable ADC1 interrupt
    HAL_NVIC_SetPriority(ADC1_IRQn, ADC_IRQ_PRIORITY, 0);
    HAL_NVIC_EnableIRQ(ADC1_IRQn);
}

/**
 * @brief Initializes the ADC1 peripheral.
 *
 * This function configures the ADC1 peripheral with the specified settings.
 * The ADC uses timer-triggered conversions with DMA for all operations.
 * Oversampling is available for all configurations.
 */ // NOLINTNEXTLINE(readability-function-cognitive-complexity)
void ADC_LL_init(ADC_LL_Config const *config)
{
    if (config == nullptr || config->output_buffer == nullptr ||
        config->buffer_size == 0) {
        THROW(ERROR_INVALID_ARGUMENT);
        return;
    }
    if (adc_num == ADC_1) {
        if (g_adc1_initialized) {
            THROW(ERROR_RESOURCE_BUSY);
            return;
        }
        if (adc_handle_initialization(ADC_1, adc_trigger_timer) != HAL_OK) {
            THROW(ERROR_HARDWARE_FAULT);
            return;
        }
        if (adc_channel_configuration(ADC_1, adc_buf, sz) != HAL_OK) {
            THROW(ERROR_HARDWARE_FAULT);
            return;
        }
        g_adc1_initialized = true; // Set the ADC1 initialized flag

    } else if (adc_num == ADC_2) {
        if (g_adc2_initialized) {
            THROW(ERROR_RESOURCE_BUSY);
            return;
        }
        if (adc_handle_initialization(ADC_2, adc_trigger_timer) != HAL_OK) {
            THROW(ERROR_HARDWARE_FAULT);
            return;
        }
        if (adc_channel_configuration(ADC_2, adc_buf, sz) != HAL_OK) {
            THROW(ERROR_HARDWARE_FAULT);
            return;
        }
        g_adc2_initialized = true; // Set the ADC2 initialized flag

    // Validate oversampling ratio
    if (config->oversampling_ratio != 1 && config->oversampling_ratio != 2 &&
        config->oversampling_ratio != 4 && config->oversampling_ratio != 8 &&
        config->oversampling_ratio != 16 && config->oversampling_ratio != 32 &&
        config->oversampling_ratio != 64 && config->oversampling_ratio != 128 &&
        config->oversampling_ratio != 256) {
        THROW(ERROR_INVALID_ARGUMENT);
        return;
    }

    // Initialize the ADC handle
    ADCInstance *instance = &g_adc_instance;
    instance->current_channel = config->channel;
    instance->oversampling_ratio = config->oversampling_ratio;

    // Initialize the ADC peripheral
    instance->adc_handle->Instance = ADC1;
    instance->adc_handle->Init.ClockPrescaler =
        ADC_CLOCK_ASYNC_DIV1; // ADC clock and prescaler
    instance->adc_handle->Init.Resolution = ADC_RESOLUTION_12B;
    instance->adc_handle->Init.DataAlign = ADC_DATAALIGN_RIGHT;
    instance->adc_handle->Init.ScanConvMode = DISABLE;
    instance->adc_handle->Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    instance->adc_handle->Init.LowPowerAutoWait = DISABLE;
    instance->adc_handle->Init.ContinuousConvMode = DISABLE;
    instance->adc_handle->Init.NbrOfConversion = 1;
    instance->adc_handle->Init.DiscontinuousConvMode = DISABLE;

    // Set external trigger based on input parameter
    instance->adc_handle->Init.ExternalTrigConv =
        get_hal_trigger_source(config->trigger_source);
    instance->adc_handle->Init.ExternalTrigConvEdge =
        ADC_EXTERNALTRIGCONVEDGE_RISING;

    instance->adc_handle->Init.SamplingMode = ADC_SAMPLING_MODE_NORMAL;
    instance->adc_handle->Init.DMAContinuousRequests = DISABLE;
    instance->adc_handle->Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;

    // Configure oversampling if ratio > 1
    if (config->oversampling_ratio > 1) {
        instance->adc_handle->Init.OversamplingMode = ENABLE;
        instance->adc_handle->Init.Oversampling.Ratio =
            config->oversampling_ratio;
        instance->adc_handle->Init.Oversampling.RightBitShift =
            ADC_RIGHTBITSHIFT_NONE;
        instance->adc_handle->Init.Oversampling.TriggeredMode =
            ADC_TRIGGEREDMODE_SINGLE_TRIGGER;
        instance->adc_handle->Init.Oversampling.OversamplingStopReset =
            ADC_REGOVERSAMPLING_CONTINUED_MODE;
    } else {
        instance->adc_handle->Init.OversamplingMode = DISABLE;
    }

    if (HAL_ADC_Init(&g_hadc) != HAL_OK) {
        THROW(ERROR_HARDWARE_FAULT);
    }

    // Configure the ADC channel
    instance->adc_config = g_config;
    g_config.Channel = get_hal_adc_channel(config->channel);
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
    instance->adc_buffer_data = config->output_buffer;
    instance->adc_buffer_size = config->buffer_size;
    instance->initialized = true;
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

    ADCInstance *instance = &g_adc_instance;

    // Stop the ADC conversion
    HAL_ADC_Stop_DMA(&g_hadc);

    // Disable interrupts
    HAL_NVIC_DisableIRQ(ADC1_IRQn);
    HAL_NVIC_DisableIRQ(GPDMA1_Channel6_IRQn);

    // Deinitialize the ADC peripheral
    if (HAL_ADC_DeInit(instance->adc_handle) != HAL_OK) {
        THROW(ERROR_HARDWARE_FAULT);
    }

    // Clear instance data
    instance->adc_buffer_data = nullptr;
    instance->adc_buffer_size = 0;
    instance->adc_complete_callback = nullptr;
    instance->current_channel = ADC_LL_CHANNEL_0;
    instance->oversampling_ratio = 1;
    instance->initialized = false;
}

/**
 * @brief Starts an ADC conversion.
 *
 * Starts timer-triggered conversions with DMA.
 *
 */
void ADC_LL_start(ADC_Num adc_num)
{
    if (!g_adc_instance.initialized) {
        THROW(ERROR_RESOURCE_UNAVAILABLE);
        return;
    }

    __HAL_ADC_CLEAR_FLAG(&g_hadc, ADC_FLAG_EOC | ADC_FLAG_EOS | ADC_FLAG_OVR);

    // Start DMA conversion
    if (HAL_ADC_Start_DMA(
            instance->adc_handle,
            instance->adc_buffer_data,
            instance->adc_buffer_size
        ) != HAL_OK) {
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
void ADC_LL_stop(ADC_Num adc_num)
{
    if (!g_adc_instance.initialized) {
        return;
    }

    // Stop DMA conversion
    HAL_ADC_Stop_DMA(&g_hadc);
}

    ADCInstance *instance = &g_adc_instances[adc_num];
    if (instance->adc_handle == nullptr) {
        THROW(ERROR_DEVICE_NOT_READY);
        return;
    }

    // Stop the ADC conversion
    if (g_adc_multimode) {
        if (HAL_ADCEx_MultiModeStop_DMA(instance->adc_handle) != HAL_OK) {
            THROW(ERROR_HARDWARE_FAULT);
            return;
        } // Stop ADC in multimode DMA
    }
    if (HAL_ADC_Stop_DMA(instance->adc_handle) != HAL_OK) {
        THROW(ERROR_HARDWARE_FAULT);
        return;
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

/**
 * @brief Gets the maximum ADC sample rate.
 *
 * This function calculates and returns the maximum ADC sample rate based on:
 * - Sample time (currently 12.5 clock cycles)
 * - Conversion time (12.5 clock cycles for 12-bit resolution)
 * - ADC clock frequency
 * - Clock prescaler (currently 1)
 *
 * @return The maximum sample rate in Hz, or 0 if ADC is not initialized or error occurs.
 */
uint32_t ADC_LL_get_sample_rate(void)
{
    // Check if ADC is initialized
    if (!g_adc_instance.initialized) {
        return 0;
    }

    // Get ADC clock frequency
    uint32_t adc_clock_hz = PLATFORM_get_peripheral_clock_speed(PLATFORM_CLOCK_ADC1);
    if (adc_clock_hz == 0) {
        return 0;
    }

    // Lookup tables for conversion time, sample time, and prescaler
    typedef struct { uint32_t key; unsigned value; } Lookup;

    static const Lookup conv_time_table[] = {
        { ADC_RESOLUTION_12B, 25 }, // 12.5 cycles * 2
        { ADC_RESOLUTION_10B, 21 }, // 10.5 cycles * 2
        { ADC_RESOLUTION_8B, 17 }, // 8.5 cycles * 2
        { ADC_RESOLUTION_6B, 13 }, // 6.5 cycles * 2
    };
    static const Lookup sample_time_table[] = {
        { ADC_SAMPLETIME_2CYCLES_5, 5 }, // 2.5 cycles * 2
        { ADC_SAMPLETIME_6CYCLES_5, 13 }, // 6.5 cycles * 2
        { ADC_SAMPLETIME_12CYCLES_5, 25 }, // 12.5 cycles * 2
        { ADC_SAMPLETIME_24CYCLES_5, 49 }, // 24.5 cycles * 2
        { ADC_SAMPLETIME_47CYCLES_5, 95 }, // 47.5 cycles * 2
        { ADC_SAMPLETIME_92CYCLES_5, 185 }, // 92.5 cycles * 2
        { ADC_SAMPLETIME_247CYCLES_5, 495 }, // 247.5 cycles * 2
        { ADC_SAMPLETIME_640CYCLES_5, 1281 }, // 640.5 cycles * 2
    };
    static const Lookup prescaler_table[] = {
        { ADC_CLOCK_SYNC_PCLK_DIV1, 1 },
        { ADC_CLOCK_SYNC_PCLK_DIV2, 2 },
        { ADC_CLOCK_SYNC_PCLK_DIV4, 4 },
        { ADC_CLOCK_ASYNC_DIV1, 1 },
        { ADC_CLOCK_ASYNC_DIV2, 2 },
        { ADC_CLOCK_ASYNC_DIV4, 4 },
        { ADC_CLOCK_ASYNC_DIV8, 8 },
        { ADC_CLOCK_ASYNC_DIV16, 16 },
        { ADC_CLOCK_ASYNC_DIV32, 32 },
        { ADC_CLOCK_ASYNC_DIV64, 64 },
        { ADC_CLOCK_ASYNC_DIV128, 128 },
        { ADC_CLOCK_ASYNC_DIV256, 256 },
    };

    unsigned conv_time_cycles_2x = 0;
    unsigned sample_time_cycles_2x = 0;
    unsigned prescaler = 0;
    size_t i;

    // Lookup conversion time
    for (i = 0; i < sizeof(conv_time_table)/sizeof(conv_time_table[0]); ++i) {
        if (g_adc_instance.adc_handle->Init.Resolution == conv_time_table[i].key) {
            conv_time_cycles_2x = conv_time_table[i].value;
            break;
        }
    }
    if (conv_time_cycles_2x == 0) {
        LOG_ERROR("ADC_LL_get_sample_rate: Unsupported ADC resolution");
        return 0;
    }

    // Lookup sample time
    for (i = 0; i < sizeof(sample_time_table)/sizeof(sample_time_table[0]); ++i) {
        if (g_adc_instance.adc_config.SamplingTime == sample_time_table[i].key) {
            sample_time_cycles_2x = sample_time_table[i].value;
            break;
        }
    }
    if (sample_time_cycles_2x == 0) {
        LOG_ERROR("ADC_LL_get_sample_rate: Unsupported ADC sample time");
        return 0;
    }

    // Lookup prescaler
    for (i = 0; i < sizeof(prescaler_table)/sizeof(prescaler_table[0]); ++i) {
        if (g_adc_instance.adc_handle->Init.ClockPrescaler == prescaler_table[i].key) {
            prescaler = prescaler_table[i].value;
            break;
        }
    }
    if (prescaler == 0) {
        LOG_ERROR("ADC_LL_get_sample_rate: Unsupported ADC clock prescaler");
        return 0;
    }

    // Calculate sample rate: Sample Rate = ADC_Clock_Rate / (Total_Cycles * Prescaler)
    uint32_t sample_rate_hz = (uint32_t)(
        adc_clock_hz
        / ((sample_time_cycles_2x + conv_time_cycles_2x) * prescaler)
    );

    return sample_rate_hz;
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    (void)hadc; // Suppress unused parameter warning

    if (g_adc_instance.adc_complete_callback != nullptr) {
        // Call the user-defined callback
        g_adc_instance.adc_complete_callback();
    }
}

void ADC1_IRQHandler(void) { HAL_ADC_IRQHandler(&g_hadc1); }

void ADC2_IRQHandler(void) { HAL_ADC_IRQHandler(&g_hadc2); }

void GPDMA1_Channel6_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&g_hdma_adc); // Handle DMA interrupts
}

/**
 * @brief ADC error callback.
 *
 * This function is called by the HAL when an ADC error occurs.
 * It simply logs the error code.
 *
 * @param hadc Pointer to the ADC handle structure.
 */
void HAL_ADC_ErrorCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc == nullptr) {
        LOG_ERROR("HAL_ADC_ErrorCallback: hadc is NULL");
        return;
    }
    uint32_t error_code = HAL_ADC_GetError(hadc);
    LOG_ERROR("HAL_ADC_ErrorCallback: error code = 0x%08lX", error_code);
}
