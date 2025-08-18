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
    ADC_HandleTypeDef
        *adc_handles[MAX_SIMULTANEOUS_CHANNELS]; // Pointers to ADC handles
    ADC_ChannelConfTypeDef
        adc_configs[MAX_SIMULTANEOUS_CHANNELS]; // ADC channel configurations
    DMA_HandleTypeDef *dma_handle; // Pointer to DMA handle
    uint16_t *buffer_data; // Pointer to ADC data buffer
    uint32_t buffer_size; // Size of the ADC data buffer (per channel)
    ADC_LL_CompleteCallback complete_callback; // Callback for ADC completion
    ADC_LL_Channel channels[MAX_SIMULTANEOUS_CHANNELS]; // ADC channels
    uint8_t channel_count; // Number of channels being used
    ADC_LL_Mode mode; // Current ADC mode
    uint32_t oversampling_ratio; // Oversampling ratio
    bool initialized; // Flag to indicate if the ADC is initialized
} ADCInstance;

static ADC_HandleTypeDef g_hadc = { nullptr };

static ADC_HandleTypeDef g_hadc2 = { nullptr };

static ADC_ChannelConfTypeDef g_config = { 0 };

static ADC_ChannelConfTypeDef g_config2 = { 0 };

static DMA_HandleTypeDef g_hdma_adc = { nullptr };

static DMA_HandleTypeDef g_hdma_adc1_dual = { nullptr };

typedef struct {
    GPIO_TypeDef *gpio_port;
    uint16_t gpio_pin;
} ADC_LL_PinConfig;

static ADCInstance g_adc_instance = {
    .adc_handles = { &g_hadc, &g_hadc2 },
    .dma_handle = &g_hdma_adc,
    .channels = { ADC_LL_CHANNEL_0, ADC_LL_CHANNEL_1 },
    .channel_count = 1,
    .mode = ADC_LL_MODE_SINGLE,
    .oversampling_ratio = 1,
    .initialized = false,
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
 * @brief Enables required clocks for ADC GPIO pins.
 *
 * @param pin_config GPIO pin configuration.
 */
static void enable_gpio_clocks(ADC_LL_PinConfig const *pin_config)
{
    if (pin_config->gpio_port == GPIOA) {
        __HAL_RCC_GPIOA_CLK_ENABLE();
    } else if (pin_config->gpio_port == GPIOB) {
        __HAL_RCC_GPIOB_CLK_ENABLE();
    } else if (pin_config->gpio_port == GPIOC) {
        __HAL_RCC_GPIOC_CLK_ENABLE();
    }
}

/**
 * @brief Configures GPIO pin for ADC input.
 *
 * @param pin_config GPIO pin configuration.
 */
static void configure_adc_gpio(ADC_LL_PinConfig const *pin_config)
{
    GPIO_InitTypeDef gpio_init = { 0 };

    enable_gpio_clocks(pin_config);

    gpio_init.Pin = pin_config->gpio_pin;
    gpio_init.Mode = GPIO_MODE_ANALOG;
    gpio_init.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(pin_config->gpio_port, &gpio_init);
}

/**
 * @brief Configures DMA handle with common settings for dual mode.
 *
 * @param hdma Pointer to DMA handle.
 * @param channel DMA channel instance.
 * @param request DMA request.
 */
static void configure_dual_mode_dma(
    DMA_HandleTypeDef *hdma,
    DMA_Channel_TypeDef *channel,
    uint32_t request
)
{
    hdma->Instance = channel;
    hdma->Init.Request = request;
    hdma->Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    hdma->Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma->Init.SrcInc = DMA_SINC_FIXED;
    hdma->Init.DestInc = DMA_DINC_INCREMENTED;
    hdma->Init.SrcDataWidth = DMA_SRC_DATAWIDTH_WORD; // 32-bit for dual ADC
    hdma->Init.DestDataWidth = DMA_DEST_DATAWIDTH_WORD;
    hdma->Init.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
    hdma->Init.SrcBurstLength = 1;
    hdma->Init.DestBurstLength = 1;
    hdma->Init.TransferAllocatedPort =
        (DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0);
    hdma->Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    hdma->Init.Mode = DMA_NORMAL;

    if (HAL_DMA_Init(hdma) != HAL_OK) {
        THROW(ERROR_HARDWARE_FAULT);
    }
}

/**
 * @brief Configures DMA handle for single mode.
 *
 * @param hdma Pointer to DMA handle.
 * @param channel DMA channel instance.
 * @param request DMA request.
 */
static void configure_single_mode_dma(
    DMA_HandleTypeDef *hdma,
    DMA_Channel_TypeDef *channel,
    uint32_t request
)
{
    hdma->Instance = channel;
    hdma->Init.Request = request;
    hdma->Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    hdma->Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma->Init.SrcInc = DMA_SINC_FIXED;
    hdma->Init.DestInc = DMA_DINC_INCREMENTED;
    hdma->Init.SrcDataWidth = DMA_SRC_DATAWIDTH_HALFWORD;
    hdma->Init.DestDataWidth = DMA_DEST_DATAWIDTH_HALFWORD;
    hdma->Init.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
    hdma->Init.SrcBurstLength = 1;
    hdma->Init.DestBurstLength = 1;
    hdma->Init.TransferAllocatedPort =
        (DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0);
    hdma->Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    hdma->Init.Mode = DMA_NORMAL;

    if (HAL_DMA_Init(hdma) != HAL_OK) {
        THROW(ERROR_HARDWARE_FAULT);
    }
}

/**
 * @brief Enables DMA and ADC interrupts for dual mode.
 *
 * @param hadc Pointer to ADC handle structure.
 */
static void enable_dual_mode_interrupts(ADC_HandleTypeDef *hadc)
{
    // Enable DMA interrupt
    HAL_NVIC_SetPriority(GPDMA1_Channel7_IRQn, ADC_IRQ_PRIORITY, 1);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel7_IRQn);

    // Enable ADC interrupts
    if (hadc->Instance == ADC1) {
        HAL_NVIC_SetPriority(ADC1_IRQn, ADC_IRQ_PRIORITY, 0);
        HAL_NVIC_EnableIRQ(ADC1_IRQn);
    } else if (hadc->Instance == ADC2) {
        HAL_NVIC_SetPriority(ADC2_IRQn, ADC_IRQ_PRIORITY, 0);
        HAL_NVIC_EnableIRQ(ADC2_IRQn);
    }
}

/**
 * @brief Enables DMA and ADC interrupts for single mode.
 */
static void enable_single_mode_interrupts(void)
{
    // Enable DMA interrupt
    HAL_NVIC_SetPriority(GPDMA1_Channel6_IRQn, ADC_IRQ_PRIORITY, 1);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel6_IRQn);

    // Enable ADC1 interrupt
    HAL_NVIC_SetPriority(ADC1_IRQn, ADC_IRQ_PRIORITY, 0);
    HAL_NVIC_EnableIRQ(ADC1_IRQn);
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
    ADC_LL_PinConfig pin_config;

    // Enable ADC clocks
    __HAL_RCC_ADC_CLK_ENABLE();

    // Get pin configuration based on ADC instance
    if (hadc->Instance == ADC1) {
        pin_config = get_pin_config(g_adc_instance.channels[0]);
    } else if (hadc->Instance == ADC2) {
        pin_config = get_pin_config(g_adc_instance.channels[1]);
    } else {
        THROW(ERROR_INVALID_ARGUMENT);
    }

    // Configure GPIO pin for ADC input
    configure_adc_gpio(&pin_config);

    if (g_adc_instance.channel_count > 1) {
        // Dual mode configuration
        if (hadc->Instance == ADC1) {
            // Enable DMA1 clock
            __HAL_RCC_GPDMA1_CLK_ENABLE();

            // Configure DMA for dual mode (both simultaneous and interleaved
            // use same settings)
            configure_dual_mode_dma(
                &g_hdma_adc1_dual, GPDMA1_Channel7, GPDMA1_REQUEST_ADC1
            );
            __HAL_LINKDMA(&g_hadc, DMA_Handle, g_hdma_adc1_dual);
        }

        // Enable interrupts for dual mode
        enable_dual_mode_interrupts(hadc);
    } else {
        // Single mode configuration
        // Enable DMA1 clock
        __HAL_RCC_GPDMA1_CLK_ENABLE();

        // Configure DMA for single mode
        configure_single_mode_dma(
            &g_hdma_adc, GPDMA1_Channel6, GPDMA1_REQUEST_ADC1
        );
        __HAL_LINKDMA(&g_hadc, DMA_Handle, g_hdma_adc);

        // Enable interrupts for single mode
        enable_single_mode_interrupts();
    }
}

/**
 * @brief Validates ADC configuration parameters.
 *
 * @param config ADC configuration to validate.
 */
static void validate_adc_config(ADC_LL_Config const *config)
{
    if (config == nullptr || config->buffer_size == 0) {
        THROW(ERROR_INVALID_ARGUMENT);
    }

    // Validate channel count and mode consistency
    if (config->channel_count == 0 ||
        config->channel_count > MAX_SIMULTANEOUS_CHANNELS) {
        THROW(ERROR_INVALID_ARGUMENT);
    }

    // Validate single-channel input in single- and interleaved modes
    if ((config->mode == ADC_LL_MODE_SINGLE ||
         config->mode == ADC_LL_MODE_INTERLEAVED) &&
        config->channel_count != 1) {
        THROW(ERROR_INVALID_ARGUMENT);
    }

    // Validate dual-channel input in simultaneous mode
    if (config->mode == ADC_LL_MODE_SIMULTANEOUS &&
        config->channel_count != 2) {
        THROW(ERROR_INVALID_ARGUMENT);
    }

    // Validate buffers
    if (config->output_buffer == nullptr) {
        THROW(ERROR_INVALID_ARGUMENT);
    }

    if (g_adc_instance.initialized) {
        THROW(ERROR_RESOURCE_BUSY);
    }

    // Validate oversampling ratio
    if (config->oversampling_ratio != 1 && config->oversampling_ratio != 2 &&
        config->oversampling_ratio != 4 && config->oversampling_ratio != 8 &&
        config->oversampling_ratio != 16 && config->oversampling_ratio != 32 &&
        config->oversampling_ratio != 64 && config->oversampling_ratio != 128 &&
        config->oversampling_ratio != 256) {
        THROW(ERROR_INVALID_ARGUMENT);
    }
}

/**
 * @brief Initializes ADC instance with configuration data.
 *
 * @param config ADC configuration.
 */
static void initialize_adc_instance(ADC_LL_Config const *config)
{
    ADCInstance *instance = &g_adc_instance;
    for (int i = 0; i < config->channel_count; i++) {
        instance->channels[i] = config->channels[i];
    }
    instance->buffer_data = config->output_buffer;
    instance->channel_count = config->channel_count;
    instance->mode = config->mode;
    instance->buffer_size = config->buffer_size;
    instance->oversampling_ratio = config->oversampling_ratio;
    instance->initialized = true; // Set before MSP init to configure mode
}

/**
 * @brief Configures oversampling for an ADC handle.
 *
 * @param adc_handle ADC handle to configure.
 * @param oversampling_ratio Oversampling ratio to apply.
 */
static void configure_adc_oversampling(
    ADC_HandleTypeDef *adc_handle,
    uint32_t oversampling_ratio
)
{
    if (oversampling_ratio > 1) {
        adc_handle->Init.OversamplingMode = ENABLE;
        adc_handle->Init.Oversampling.Ratio = oversampling_ratio;
        adc_handle->Init.Oversampling.RightBitShift = ADC_RIGHTBITSHIFT_NONE;
        adc_handle->Init.Oversampling.TriggeredMode =
            ADC_TRIGGEREDMODE_SINGLE_TRIGGER;
        adc_handle->Init.Oversampling.OversamplingStopReset =
            ADC_REGOVERSAMPLING_CONTINUED_MODE;
    } else {
        adc_handle->Init.OversamplingMode = DISABLE;
    }
}

/**
 * @brief Sets common ADC initialization parameters.
 *
 * @param adc_handle ADC handle to configure.
 */
static void set_common_adc_init_params(ADC_HandleTypeDef *adc_handle)
{
    adc_handle->Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
    adc_handle->Init.Resolution = ADC_RESOLUTION_12B;
    adc_handle->Init.DataAlign = ADC_DATAALIGN_RIGHT;
    adc_handle->Init.ScanConvMode = DISABLE;
    adc_handle->Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    adc_handle->Init.LowPowerAutoWait = DISABLE;
    adc_handle->Init.ContinuousConvMode = DISABLE;
    adc_handle->Init.NbrOfConversion = 1;
    adc_handle->Init.DiscontinuousConvMode = DISABLE;
    adc_handle->Init.SamplingMode = ADC_SAMPLING_MODE_NORMAL;
    adc_handle->Init.DMAContinuousRequests = DISABLE;
    adc_handle->Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
}

/**
 * @brief Configures ADC1 initialization parameters.
 *
 * @param config ADC configuration.
 */
static void configure_adc1_init(ADC_LL_Config const *config)
{
    ADCInstance *instance = &g_adc_instance;

    instance->adc_handles[0]->Instance = ADC1;
    set_common_adc_init_params(instance->adc_handles[0]);

    instance->adc_handles[0]->Init.ExternalTrigConv =
        get_hal_trigger_source(config->trigger_source);
    instance->adc_handles[0]->Init.ExternalTrigConvEdge =
        ADC_EXTERNALTRIGCONVEDGE_RISING;

    configure_adc_oversampling(
        instance->adc_handles[0], config->oversampling_ratio
    );
}

/**
 * @brief Configures ADC2 initialization parameters for dual modes.
 *
 * @param config ADC configuration.
 */
static void configure_adc2_init(ADC_LL_Config const *config)
{
    ADCInstance *instance = &g_adc_instance;

    instance->adc_handles[1]->Instance = ADC2;
    set_common_adc_init_params(instance->adc_handles[1]);

    // Configure trigger based on mode
    if (config->mode == ADC_LL_MODE_INTERLEAVED) {
        instance->adc_handles[1]->Init.ExternalTrigConv =
            get_hal_trigger_source(config->trigger_source);
        instance->adc_handles[1]->Init.ExternalTrigConvEdge =
            ADC_EXTERNALTRIGCONVEDGE_FALLING;
    } else {
        // Simultaneous mode - ADC2 is slave in multimode
        instance->adc_handles[1]->Init.ExternalTrigConv = ADC_SOFTWARE_START;
        instance->adc_handles[1]->Init.ExternalTrigConvEdge =
            ADC_EXTERNALTRIGCONVEDGE_NONE;
    }

    configure_adc_oversampling(
        instance->adc_handles[1], config->oversampling_ratio
    );
}

/**
 * @brief Configures ADC channel parameters.
 *
 * @param adc_handle ADC handle to configure.
 * @param channel_config ADC channel configuration structure.
 * @param channel ADC channel to configure.
 */
static void configure_adc_channel(
    ADC_HandleTypeDef *adc_handle,
    ADC_ChannelConfTypeDef *channel_config,
    ADC_LL_Channel channel
)
{
    *channel_config = (ADC_ChannelConfTypeDef){ 0 };
    channel_config->Channel = get_hal_adc_channel(channel);
    channel_config->Rank = ADC_REGULAR_RANK_1;
    channel_config->SamplingTime = ADC_SAMPLETIME_12CYCLES_5;
    channel_config->SingleDiff = ADC_SINGLE_ENDED;
    channel_config->OffsetNumber = ADC_OFFSET_NONE;
    channel_config->Offset = 0;

    if (HAL_ADC_ConfigChannel(adc_handle, channel_config) != HAL_OK) {
        THROW(ERROR_HARDWARE_FAULT);
    }
}

/**
 * @brief Configures dual mode (simultaneous or interleaved) settings.
 *
 * @param config ADC configuration.
 */
static void configure_dual_mode(ADC_LL_Config const *config)
{
    ADC_MultiModeTypeDef multimode = { 0 };

    if (config->mode == ADC_LL_MODE_INTERLEAVED) {
        multimode.Mode = ADC_DUALMODE_INTERL;
    } else {
        multimode.Mode = ADC_DUALMODE_REGSIMULT;
    }
    multimode.DMAAccessMode = ADC_DMAACCESSMODE_12_10_BITS;
    multimode.TwoSamplingDelay = ADC_TWOSAMPLINGDELAY_1CYCLE;

    if (HAL_ADCEx_MultiModeConfigChannel(&g_hadc, &multimode) != HAL_OK) {
        THROW(ERROR_HARDWARE_FAULT);
    }
}

/**
 * @brief Performs ADC calibration for configured ADCs.
 *
 * @param config ADC configuration.
 */
static void calibrate_adc(ADC_LL_Config const *config)
{
    if (HAL_ADCEx_Calibration_Start(&g_hadc, ADC_SINGLE_ENDED) != HAL_OK) {
        THROW(ERROR_HARDWARE_FAULT);
    }

    if (config->channel_count > 1) {
        if (HAL_ADCEx_Calibration_Start(&g_hadc2, ADC_SINGLE_ENDED) != HAL_OK) {
            THROW(ERROR_HARDWARE_FAULT);
        }
    }
}

/**
 * @brief Initializes the ADC peripheral(s).
 *
 * This function configures the ADC peripheral(s) with the specified settings.
 * For single-channel mode, only ADC1 is used. For dual-channel modes,
 * both ADC1 and ADC2 are configured.
 * The ADC uses timer-triggered conversions with DMA for all operations.
 * Oversampling is available for all configurations.
 */
void ADC_LL_init(ADC_LL_Config const *config)
{
    validate_adc_config(config);
    initialize_adc_instance(config);

    ADCInstance *instance = &g_adc_instance;

    // Initialize and configure ADC1
    configure_adc1_init(config);
    if (HAL_ADC_Init(&g_hadc) != HAL_OK) {
        THROW(ERROR_HARDWARE_FAULT);
    }
    configure_adc_channel(
        instance->adc_handles[0], &g_config, config->channels[0]
    );

    // Initialize ADC2 for dual modes
    if (config->mode == ADC_LL_MODE_SIMULTANEOUS ||
        config->mode == ADC_LL_MODE_INTERLEAVED) {
        configure_adc2_init(config);
        if (HAL_ADC_Init(&g_hadc2) != HAL_OK) {
            THROW(ERROR_HARDWARE_FAULT);
        }
        configure_adc_channel(
            instance->adc_handles[1], &g_config2, config->channels[1]
        );
        configure_dual_mode(config);
    }

    calibrate_adc(config);
}

/**
 * @brief Deinitializes the ADC peripheral(s).
 *
 */
void ADC_LL_deinit(ADC_Num adc_num)
{
    if (!g_adc_instance.initialized) {
        THROW(ERROR_RESOURCE_UNAVAILABLE);
    }

    ADCInstance *instance = &g_adc_instance;

    // Stop ADC conversions based on mode
    if (instance->mode == ADC_LL_MODE_SIMULTANEOUS ||
        instance->mode == ADC_LL_MODE_INTERLEAVED) {
        // Stop multimode DMA transfer
        HAL_ADCEx_MultiModeStop_DMA(&g_hadc);
    } else {
        // Single mode
        HAL_ADC_Stop_DMA(&g_hadc);
    }

    // Disable interrupts
    HAL_NVIC_DisableIRQ(ADC1_IRQn);
    if (instance->channel_count > 1) {
        HAL_NVIC_DisableIRQ(ADC2_IRQn);
        HAL_NVIC_DisableIRQ(GPDMA1_Channel7_IRQn);
    } else {
        HAL_NVIC_DisableIRQ(GPDMA1_Channel6_IRQn);
    }

    // Deinitialize ADC peripherals
    if (HAL_ADC_DeInit(instance->adc_handles[0]) != HAL_OK) {
        THROW(ERROR_HARDWARE_FAULT);
    }
    if (instance->channel_count > 1) {
        if (HAL_ADC_DeInit(instance->adc_handles[1]) != HAL_OK) {
            THROW(ERROR_HARDWARE_FAULT);
        }
    }

    // Clear instance data
    instance->buffer_data = nullptr;
    for (int i = 0; i < MAX_SIMULTANEOUS_CHANNELS; i++) {
        instance->channels[i] = ADC_LL_CHANNEL_0;
    }
    instance->buffer_size = 0;
    instance->complete_callback = nullptr;
    instance->channel_count = 1;
    instance->mode = ADC_LL_MODE_SINGLE;
    instance->oversampling_ratio = 1;
    instance->initialized = false;
}

/**
 * @brief Starts ADC conversion(s).
 *
 * Starts timer-triggered conversions with DMA.
 *
 */
void ADC_LL_start(ADC_Num adc_num)
{
    if (!g_adc_instance.initialized) {
        THROW(ERROR_RESOURCE_UNAVAILABLE);
    }

    __HAL_ADC_CLEAR_FLAG(&g_hadc, ADC_FLAG_EOC | ADC_FLAG_EOS | ADC_FLAG_OVR);
    if (g_adc_instance.channel_count > 1) {
        __HAL_ADC_CLEAR_FLAG(
            &g_hadc2, ADC_FLAG_EOC | ADC_FLAG_EOS | ADC_FLAG_OVR
        );
    }

    if (g_adc_instance.mode == ADC_LL_MODE_SIMULTANEOUS ||
        g_adc_instance.mode == ADC_LL_MODE_INTERLEAVED) {
        // Start multimode DMA conversion (both simultaneous and interleaved use
        // multimode)
        uint32_t dma_buffer_size = g_adc_instance.buffer_size;
        if (g_adc_instance.mode == ADC_LL_MODE_SIMULTANEOUS ||
            g_adc_instance.mode == ADC_LL_MODE_INTERLEAVED) {
            dma_buffer_size *= 2; // Double buffer size for dual-channel modes
        }

        if (HAL_ADCEx_MultiModeStart_DMA(
                &g_hadc,
                (uint32_t *)g_adc_instance
                    .buffer_data, // Cast to uint32_t for multimode
                dma_buffer_size /
                    2 // Divide by 2 because we're transferring 32-bit words
            ) != HAL_OK) {
            THROW(ERROR_HARDWARE_FAULT);
        }
    } else {
        // Single mode DMA conversion
        if (HAL_ADC_Start_DMA(
                &g_hadc,
                (uint32_t *)g_adc_instance.buffer_data,
                g_adc_instance.buffer_size
            ) != HAL_OK) {
            THROW(ERROR_HARDWARE_FAULT);
        }
    }
}

/**
 * @brief Stops the ADC conversion(s).
 *
 * This function stops the ongoing ADC conversion process. It can be called
 * to halt conversions before deinitializing the ADC or when no longer needed.
 */
void ADC_LL_stop(ADC_Num adc_num)
{
    if (!g_adc_instance.initialized) {
        return;
    }

    if (g_adc_instance.mode == ADC_LL_MODE_SIMULTANEOUS ||
        g_adc_instance.mode == ADC_LL_MODE_INTERLEAVED) {
        // Stop multimode DMA conversion
        HAL_ADCEx_MultiModeStop_DMA(&g_hadc);
    } else {
        // Single mode
        HAL_ADC_Stop_DMA(&g_hadc);
    }
}

/**
 * @brief Sets the callback for ADC completion events.
 *
 * This function sets a user-defined callback that will be called when
 * the ADC conversion is complete.
 *
 * @param callback Pointer to the callback function to be set.
 */
void ADC_LL_set_complete_callback(
    ADC_Num adc_num,
    ADC_LL_CompleteCallback callback
)
{
    g_adc_instance.complete_callback = callback;
}

/**
 * @brief Gets the current ADC operation mode.
 *
 * @return Current ADC mode (single, simultaneous, or interleaved).
 */
ADC_LL_Mode ADC_LL_get_mode(void)
{
    if (g_adc_instance.initialized) {
        return g_adc_instance.mode;
    }
    return ADC_LL_MODE_SINGLE;
}

/**
 * @brief Gets the maximum ADC sample rate.
 *
 * This function calculates and returns the maximum ADC sample rate based on:
 * - Sample time
 * - Conversion time
 * - ADC clock frequency
 * - Clock prescale
 *
 * @return The maximum sample rate in Hz, or 0 if ADC is not initialized or
 * error occurs.
 */
uint32_t ADC_LL_get_sample_rate(void)
{
    // Check if ADC is initialized
    if (!g_adc_instance.initialized) {
        return 0;
    }

    // Get ADC clock frequency
    uint32_t adc_clock_hz =
        PLATFORM_get_peripheral_clock_speed(PLATFORM_CLOCK_ADC1);
    if (adc_clock_hz == 0) {
        return 0;
    }

    // Lookup tables for conversion time, sample time, and prescaler
    typedef struct {
        uint32_t key;
        uint32_t value;
    } Lookup;

    static Lookup const conv_time_table[] = {
        { ADC_RESOLUTION_12B, 25 }, // 12.5 cycles * 2
        { ADC_RESOLUTION_10B, 21 }, // 10.5 cycles * 2
        { ADC_RESOLUTION_8B, 17 }, // 8.5 cycles * 2
        { ADC_RESOLUTION_6B, 13 }, // 6.5 cycles * 2
    };
    static Lookup const sample_time_table[] = {
        { ADC_SAMPLETIME_2CYCLES_5, 5 }, // 2.5 cycles * 2
        { ADC_SAMPLETIME_6CYCLES_5, 13 }, // 6.5 cycles * 2
        { ADC_SAMPLETIME_12CYCLES_5, 25 }, // 12.5 cycles * 2
        { ADC_SAMPLETIME_24CYCLES_5, 49 }, // 24.5 cycles * 2
        { ADC_SAMPLETIME_47CYCLES_5, 95 }, // 47.5 cycles * 2
        { ADC_SAMPLETIME_92CYCLES_5, 185 }, // 92.5 cycles * 2
        { ADC_SAMPLETIME_247CYCLES_5, 495 }, // 247.5 cycles * 2
        { ADC_SAMPLETIME_640CYCLES_5, 1281 }, // 640.5 cycles * 2
    };
    static Lookup const prescaler_table[] = {
        { ADC_CLOCK_SYNC_PCLK_DIV1, 1 }, { ADC_CLOCK_SYNC_PCLK_DIV2, 2 },
        { ADC_CLOCK_SYNC_PCLK_DIV4, 4 }, { ADC_CLOCK_ASYNC_DIV1, 1 },
        { ADC_CLOCK_ASYNC_DIV2, 2 },     { ADC_CLOCK_ASYNC_DIV4, 4 },
        { ADC_CLOCK_ASYNC_DIV8, 8 },     { ADC_CLOCK_ASYNC_DIV16, 16 },
        { ADC_CLOCK_ASYNC_DIV32, 32 },   { ADC_CLOCK_ASYNC_DIV64, 64 },
        { ADC_CLOCK_ASYNC_DIV128, 128 }, { ADC_CLOCK_ASYNC_DIV256, 256 },
    };

    uint32_t conv_time_cycles_2x = 0;
    uint32_t sample_time_cycles_2x = 0;
    uint32_t prescaler = 0;
    size_t i;

    // Lookup conversion time
    for (i = 0; i < sizeof(conv_time_table) / sizeof(conv_time_table[0]); ++i) {
        if (g_adc_instance.adc_handles[0]->Init.Resolution ==
            conv_time_table[i].key) {
            conv_time_cycles_2x = conv_time_table[i].value;
            break;
        }
    }
    if (conv_time_cycles_2x == 0) {
        LOG_ERROR("ADC_LL_get_sample_rate: Unsupported ADC resolution");
        return 0;
    }

    // Lookup sample time
    for (i = 0; i < sizeof(sample_time_table) / sizeof(sample_time_table[0]);
         ++i) {
        if (g_adc_instance.adc_configs[0].SamplingTime ==
            sample_time_table[i].key) {
            sample_time_cycles_2x = sample_time_table[i].value;
            break;
        }
    }
    if (sample_time_cycles_2x == 0) {
        LOG_ERROR("ADC_LL_get_sample_rate: Unsupported ADC sample time");
        return 0;
    }

    // Lookup prescaler
    for (i = 0; i < sizeof(prescaler_table) / sizeof(prescaler_table[0]); ++i) {
        if (g_adc_instance.adc_handles[0]->Init.ClockPrescaler ==
            prescaler_table[i].key) {
            prescaler = prescaler_table[i].value;
            break;
        }
    }
    if (prescaler == 0) {
        LOG_ERROR("ADC_LL_get_sample_rate: Unsupported ADC clock prescaler");
        return 0;
    }

    uint32_t total_cycles = (sample_time_cycles_2x + conv_time_cycles_2x) / 2;
    uint32_t sample_rate_hz = adc_clock_hz / (total_cycles * prescaler);

    return sample_rate_hz;
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (g_adc_instance.initialized &&
        g_adc_instance.complete_callback != nullptr) {
        uint32_t total_samples;

        if (g_adc_instance.mode == ADC_LL_MODE_SIMULTANEOUS ||
            g_adc_instance.mode == ADC_LL_MODE_INTERLEAVED) {
            // For multimode, we have 2 samples per conversion cycle (ADC1 +
            // ADC2)
            total_samples = g_adc_instance.buffer_size * 2;
        } else {
            // Single mode
            total_samples = g_adc_instance.buffer_size;
        }

        g_adc_instance.complete_callback(
            g_adc_instance.buffer_data, total_samples
        );
    }
}

void ADC1_IRQHandler(void) { HAL_ADC_IRQHandler(&g_hadc1); }

void ADC2_IRQHandler(void) { HAL_ADC_IRQHandler(&g_hadc2); }

void ADC2_IRQHandler(void) { HAL_ADC_IRQHandler(&g_hadc2); }

void GPDMA1_Channel6_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&g_hdma_adc); // Handle single mode DMA interrupts
}

void GPDMA1_Channel7_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&g_hdma_adc1_dual
    ); // Handle ADC1 dual mode DMA interrupts
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
