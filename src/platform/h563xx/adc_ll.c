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

#include "util/error.h"
#include "util/logging.h"
#include "util/si_prefix.h"

#include "adc_ll.h"
#include "platform.h"

enum { ADC_IRQ_PRIORITY = 1 }; // ADC interrupt priority

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
    ADC_LL_Mode mode; // Current ADC mode
    uint32_t oversampling_ratio; // Oversampling ratio
    uint32_t vref_mv; // Reference voltage in millivolts
    bool initialized; // Flag to indicate if the ADC is initialized
} ADCInstance;

static ADC_HandleTypeDef g_hadc1 = { nullptr };

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
    .adc_handles = { &g_hadc1, &g_hadc2 },
    .dma_handle = &g_hdma_adc,
    .channels = { ADC_LL_CHANNEL_0, ADC_LL_CHANNEL_1 },
    .mode = ADC_LL_MODE_SINGLE,
    .oversampling_ratio = 1,
    .vref_mv = 0,
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
    ADC_LL_PinConfig config = { nullptr };

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
 * @brief DMA transfer complete callback for multimode ADC.
 *
 * This callback is called when the DMA transfer completes in multimode.
 * Note: HAL_ADC_ConvCpltCallback is NOT called in multimode - only this DMA
 * callback.
 */
static void ADC_DMA_ConvCpltCallback(DMA_HandleTypeDef *hdma)
{
    (void)hdma;
    // For multimode, we treat this as ADC1 completion
    // since the DMA is linked to ADC1 (master)
    HAL_ADC_ConvCpltCallback(&g_hadc1);
}

/**
 * @brief DMA transfer error callback for multimode ADC.
 */
static void ADC_DMA_ErrorCallback(DMA_HandleTypeDef *hdma)
{
    (void)hdma;
    HAL_ADC_ErrorCallback(&g_hadc1);
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

    // Set DMA completion callback for multimode BEFORE HAL_DMA_Init
    // In multimode, HAL_ADC_ConvCpltCallback is NOT called - only DMA callback
    // is called
    hdma->XferCpltCallback = ADC_DMA_ConvCpltCallback;
    hdma->XferErrorCallback = ADC_DMA_ErrorCallback;

    if (HAL_DMA_Init(hdma) != HAL_OK) {
        THROW(ERROR_HARDWARE_FAULT);
    }
    __HAL_LINKDMA(&g_hadc1, DMA_Handle, g_hdma_adc1_dual);
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
    __HAL_LINKDMA(&g_hadc1, DMA_Handle, g_hdma_adc);
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
    LOG_FUNCTION_ENTRY();
    ADC_LL_PinConfig pin_config;

    // Enable ADC clock
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

    // Enable DMA clock
    __HAL_RCC_GPDMA1_CLK_ENABLE();

    if (g_adc_instance.mode == ADC_LL_MODE_SIMULTANEOUS ||
        g_adc_instance.mode == ADC_LL_MODE_INTERLEAVED) {
        // Dual mode configuration
        if (hadc->Instance == ADC1) {
            // Configure DMA for dual mode (both simultaneous and interleaved
            // use same settings)
            configure_dual_mode_dma(
                &g_hdma_adc1_dual, GPDMA1_Channel7, GPDMA1_REQUEST_ADC1
            );

            // Enable DMA interrupt
            HAL_NVIC_SetPriority(GPDMA1_Channel7_IRQn, ADC_IRQ_PRIORITY, 1);
            HAL_NVIC_EnableIRQ(GPDMA1_Channel7_IRQn);
        }
        // Note: ADC2 in multimode is slave and doesn't need DMA or separate
        // interrupts
    } else {
        // Configure DMA for single mode
        configure_single_mode_dma(
            &g_hdma_adc, GPDMA1_Channel6, GPDMA1_REQUEST_ADC1
        );

        // Enable DMA interrupts
        HAL_NVIC_SetPriority(GPDMA1_Channel6_IRQn, ADC_IRQ_PRIORITY, 1);
        HAL_NVIC_EnableIRQ(GPDMA1_Channel6_IRQn);
    }
    LOG_FUNCTION_EXIT();
}

/**
 * @brief Validates basic ADC configuration structure.
 *
 * @param config ADC configuration to validate.
 */
static void validate_adc_config_structure(ADC_LL_Config const *config)
{
    if (config == nullptr || config->buffer_size == 0) {
        THROW(ERROR_INVALID_ARGUMENT);
    }

    if (config->output_buffer == nullptr) {
        THROW(ERROR_INVALID_ARGUMENT);
    }
}

/**
 * @brief Validates oversampling ratio is a power of 2 between 1 and 256.
 *
 * @param ratio Oversampling ratio to validate.
 */
static void validate_oversampling_ratio(uint32_t ratio)
{
    // Check if ratio is a power of 2 and within valid range (1 to 256)
    if (ratio == 0 || ratio > 256 || (ratio & (ratio - 1)) != 0) {
        THROW(ERROR_INVALID_ARGUMENT);
    }
}

/**
 * @brief Validates ADC configuration parameters.
 *
 * @param config ADC configuration to validate.
 */
static void validate_adc_config(ADC_LL_Config const *config)
{
    validate_adc_config_structure(config);
    validate_oversampling_ratio(config->oversampling_ratio);

    if (g_adc_instance.initialized) {
        THROW(ERROR_RESOURCE_BUSY);
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
    // Copy channels based on mode
    uint8_t num_channels = (config->mode == ADC_LL_MODE_SIMULTANEOUS) ? 2 : 1;
    for (int i = 0; i < num_channels; i++) {
        instance->channels[i] = config->channels[i];
    }
    instance->buffer_data = config->output_buffer;
    instance->mode = config->mode;
    instance->buffer_size = config->buffer_size;
    instance->oversampling_ratio = config->oversampling_ratio;
    instance->initialized = true; // Set before MSP init to configure mode
}

/**
 * @brief Converts numeric oversampling ratio to HAL constant.
 *
 * @param ratio Numeric oversampling ratio (2, 4, 8, 16, etc.).
 * @return Corresponding HAL oversampling ratio constant.
 */
static uint32_t get_hal_oversampling_ratio(uint32_t ratio)
{
    switch (ratio) {
    case 2:
        return ADC_OVERSAMPLING_RATIO_2;
    case 4:
        return ADC_OVERSAMPLING_RATIO_4;
    case 8:
        return ADC_OVERSAMPLING_RATIO_8;
    case 16:
        return ADC_OVERSAMPLING_RATIO_16;
    case 32:
        return ADC_OVERSAMPLING_RATIO_32;
    case 64:
        return ADC_OVERSAMPLING_RATIO_64;
    case 128:
        return ADC_OVERSAMPLING_RATIO_128;
    case 256:
        return ADC_OVERSAMPLING_RATIO_256;
    default:
        return ADC_OVERSAMPLING_RATIO_2; // Default fallback
    }
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
        adc_handle->Init.Oversampling.Ratio =
            get_hal_oversampling_ratio(oversampling_ratio);
        adc_handle->Init.Oversampling.RightBitShift = ADC_RIGHTBITSHIFT_4;
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
    adc_handle->Init.Overrun = ADC_OVR_DATA_PRESERVED;
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
    channel_config->SamplingTime = ADC_SAMPLETIME_92CYCLES_5;
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

    if (HAL_ADCEx_MultiModeConfigChannel(&g_hadc1, &multimode) != HAL_OK) {
        THROW(ERROR_HARDWARE_FAULT);
    }
}

/**
 * @brief Calculates VDDA (analog supply voltage) using the internal VREFINT
 * channel.
 *
 * This function temporarily configures ADC1 to read the internal VREFINT
 * channel and calculates the actual VDDA (analog supply voltage) using the
 * factory calibration value. VREFINT is a stable ~1.21V internal reference,
 * and by measuring it we can determine what VDDA must be.
 *
 * @param instance Pointer to ADC instance structure.
 */
static void read_vdda_voltage(ADCInstance *instance)
{
    LOG_FUNCTION_ENTRY();
    ADC_ChannelConfTypeDef vref_config = { 0 };
    uint32_t vref_adc_value = 0;

    // Configure VREFINT channel
    vref_config.Channel = ADC_CHANNEL_VREFINT;
    vref_config.Rank = ADC_REGULAR_RANK_1;
    // Longer sampling for internal channels
    vref_config.SamplingTime = ADC_SAMPLETIME_247CYCLES_5;
    vref_config.SingleDiff = ADC_SINGLE_ENDED;
    vref_config.OffsetNumber = ADC_OFFSET_NONE;
    vref_config.Offset = 0;

    if (HAL_ADC_ConfigChannel(&g_hadc1, &vref_config)) {
        LOG_ERROR("Failed to configure VREFINT channel");
        THROW(ERROR_HARDWARE_FAULT);
    }

    // Temporarily switch to software trigger for VREFINT reading
    uint32_t original_trigger = g_hadc1.Init.ExternalTrigConv;
    uint32_t original_trigger_edge = g_hadc1.Init.ExternalTrigConvEdge;
    g_hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    g_hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;

    // Re-initialize ADC with software trigger
    if (HAL_ADC_Init(&g_hadc1) != HAL_OK) {
        LOG_ERROR("Failed to reconfigure ADC for software trigger");
        THROW(ERROR_HARDWARE_FAULT);
    }

    // Start ADC conversion
    if (HAL_ADC_Start(&g_hadc1)) {
        LOG_ERROR("Failed to start ADC conversion");
        THROW(ERROR_HARDWARE_FAULT);
    }

    // Wait for conversion completion
    uint32_t const sample_rate_khz = ADC_LL_get_sample_rate() / SI_KILO_INT;
    if (!sample_rate_khz) {
        LOG_ERROR(
            "Invalid sample rate for VREFINT read: %lu Hz", sample_rate_khz
        );
        THROW(ERROR_HARDWARE_FAULT);
    }
    uint32_t timeout_ms = 2 / sample_rate_khz; // 2 samples timeout
    timeout_ms = timeout_ms ? timeout_ms : 1; // Ensure at least 1 ms timeout
    LOG_DEBUG("VREFINT read timeout: %lu ms", timeout_ms);
    if (HAL_ADC_PollForConversion(&g_hadc1, timeout_ms)) {
        HAL_ADC_Stop(&g_hadc1);
        LOG_ERROR("ADC conversion timeout for VREFINT");
        THROW(ERROR_HARDWARE_FAULT);
    }

    vref_adc_value = HAL_ADC_GetValue(&g_hadc1);
    LOG_DEBUG("VREFINT ADC value: %lu", vref_adc_value);
    HAL_ADC_Stop(&g_hadc1);

    // Restore original trigger configuration
    g_hadc1.Init.ExternalTrigConv = original_trigger;
    g_hadc1.Init.ExternalTrigConvEdge = original_trigger_edge;

    // Re-initialize ADC with original trigger configuration
    if (HAL_ADC_Init(&g_hadc1) != HAL_OK) {
        LOG_ERROR("Failed to restore ADC trigger configuration");
        THROW(ERROR_HARDWARE_FAULT);
    }

    // Calculate VDDA (analog supply voltage) using the HAL macro
    // This macro calculates: VDDA = (VREFINT_CAL * 3300) / ADC_DATA
    // where VREFINT_CAL is the factory calibration value when VDDA was 3.3V
    uint32_t calculated_vdda =
        __HAL_ADC_CALC_VREFANALOG_VOLTAGE(vref_adc_value, ADC_RESOLUTION_12B);
    LOG_DEBUG("Calculated VDDA using HAL macro: %lu mV", calculated_vdda);
    LOG_DEBUG("VREFINT_CAL factory value: %lu", (uint32_t)(*VREFINT_CAL_ADDR));

    instance->vref_mv = calculated_vdda;

    LOG_FUNCTION_EXIT();
}

/**
 * @brief Performs ADC calibration for configured ADCs.
 *
 * This function should be called AFTER HAL_ADC_Init() to ensure the ADC
 * is properly initialized before calibration.
 *
 * @param config ADC configuration.
 */
static void calibrate_adc(ADC_LL_Config const *config)
{
    LOG_FUNCTION_ENTRY();

    if (HAL_ADCEx_Calibration_Start(&g_hadc1, ADC_SINGLE_ENDED) != HAL_OK) {
        THROW(ERROR_HARDWARE_FAULT);
    }

    if (config->mode == ADC_LL_MODE_SIMULTANEOUS ||
        config->mode == ADC_LL_MODE_INTERLEAVED) {
        if (HAL_ADCEx_Calibration_Start(&g_hadc2, ADC_SINGLE_ENDED) != HAL_OK) {
            THROW(ERROR_HARDWARE_FAULT);
        }
    }

    LOG_FUNCTION_EXIT();
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
    LOG_FUNCTION_ENTRY();

    validate_adc_config(config);
    initialize_adc_instance(config);

    ADCInstance *instance = &g_adc_instance;

    // Set ADC instances first (required for calibration)
    g_hadc1.Instance = ADC1;
    g_hadc2.Instance = ADC2;

    // Configure ADC1 parameters
    configure_adc1_init(config);

    // Configure ADC2 parameters for dual modes
    if (config->mode == ADC_LL_MODE_SIMULTANEOUS ||
        config->mode == ADC_LL_MODE_INTERLEAVED) {
        configure_adc2_init(config);
    }

    // Initialize ADC1
    if (HAL_ADC_Init(&g_hadc1) != HAL_OK) {
        THROW(ERROR_HARDWARE_FAULT);
    }

    // Initialize ADC2 for dual modes
    if (config->mode == ADC_LL_MODE_SIMULTANEOUS ||
        config->mode == ADC_LL_MODE_INTERLEAVED) {
        if (HAL_ADC_Init(&g_hadc2) != HAL_OK) {
            THROW(ERROR_HARDWARE_FAULT);
        }
    }

    // Perform ADC calibration after initialization
    calibrate_adc(config);

    // Read VDDA voltage
    read_vdda_voltage(instance);

    // Configure channels
    configure_adc_channel(
        instance->adc_handles[0], &g_config, config->channels[0]
    );

    // Configure ADC2 channel and dual mode for dual modes
    if (config->mode == ADC_LL_MODE_SIMULTANEOUS ||
        config->mode == ADC_LL_MODE_INTERLEAVED) {
        configure_adc_channel(
            instance->adc_handles[1], &g_config2, config->channels[1]
        );

        configure_dual_mode(config);
    }
}

/**
 * @brief Deinitializes the ADC peripheral(s).
 *
 */
void ADC_LL_deinit(void)
{
    if (!g_adc_instance.initialized) {
        THROW(ERROR_RESOURCE_UNAVAILABLE);
    }

    ADCInstance *instance = &g_adc_instance;

    // Stop ADC conversions based on mode
    if (instance->mode == ADC_LL_MODE_SIMULTANEOUS ||
        instance->mode == ADC_LL_MODE_INTERLEAVED) {
        // Stop multimode DMA transfer
        HAL_ADCEx_MultiModeStop_DMA(&g_hadc1);
    } else {
        // Single mode
        HAL_ADC_Stop_DMA(&g_hadc1);
    }

    // Disable interrupts
    HAL_NVIC_DisableIRQ(ADC1_IRQn);
    if (instance->mode == ADC_LL_MODE_SIMULTANEOUS ||
        instance->mode == ADC_LL_MODE_INTERLEAVED) {
        HAL_NVIC_DisableIRQ(ADC2_IRQn);
        HAL_NVIC_DisableIRQ(GPDMA1_Channel7_IRQn);
    } else {
        HAL_NVIC_DisableIRQ(GPDMA1_Channel6_IRQn);
    }

    // Deinitialize ADC peripherals
    if (HAL_ADC_DeInit(instance->adc_handles[0]) != HAL_OK) {
        THROW(ERROR_HARDWARE_FAULT);
    }
    if (instance->mode == ADC_LL_MODE_SIMULTANEOUS ||
        instance->mode == ADC_LL_MODE_INTERLEAVED) {
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
    instance->mode = ADC_LL_MODE_SINGLE;
    instance->oversampling_ratio = 1;
    instance->vref_mv = 0;
    instance->initialized = false;
}

/**
 * @brief Starts ADC conversion(s).
 *
 * Starts timer-triggered conversions with DMA.
 *
 */
void ADC_LL_start(void)
{
    if (!g_adc_instance.initialized) {
        THROW(ERROR_RESOURCE_UNAVAILABLE);
    }

    __HAL_ADC_CLEAR_FLAG(&g_hadc1, ADC_FLAG_EOC | ADC_FLAG_EOS | ADC_FLAG_OVR);

    if (g_adc_instance.mode == ADC_LL_MODE_SIMULTANEOUS ||
        g_adc_instance.mode == ADC_LL_MODE_INTERLEAVED) {
        __HAL_ADC_CLEAR_FLAG(
            &g_hadc2, ADC_FLAG_EOC | ADC_FLAG_EOS | ADC_FLAG_OVR
        );

        // For dual mode DMA with 32-bit transfers, buffer_size should be in
        // 32-bit words Each 32-bit word contains 2x 16-bit ADC samples (ADC1 +
        // ADC2)
        uint32_t dma_transfer_count = g_adc_instance.buffer_size / 2;

        // Start multimode DMA conversion (both simultaneous and interleaved use
        // multimode)
        if (HAL_ADCEx_MultiModeStart_DMA(
                &g_hadc1,
                (uint32_t *)g_adc_instance.buffer_data,
                dma_transfer_count
            ) != HAL_OK) {
            THROW(ERROR_HARDWARE_FAULT);
        }
    } else {
        // Single mode DMA conversion
        if (HAL_ADC_Start_DMA(
                &g_hadc1,
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
void ADC_LL_stop(void)
{
    if (!g_adc_instance.initialized) {
        return;
    }

    if (g_adc_instance.mode == ADC_LL_MODE_SIMULTANEOUS ||
        g_adc_instance.mode == ADC_LL_MODE_INTERLEAVED) {
        // Stop multimode DMA conversion
        HAL_ADCEx_MultiModeStop_DMA(&g_hadc1);
    } else {
        // Single mode
        HAL_ADC_Stop_DMA(&g_hadc1);
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
void ADC_LL_set_complete_callback(ADC_LL_CompleteCallback callback)
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
 * @brief Lookup table entry structure for ADC timing parameters.
 */
typedef struct {
    uint32_t key;
    uint32_t value;
} Lookup;

/**
 * @brief Gets conversion time cycles for ADC resolution.
 *
 * @param resolution ADC resolution setting.
 * @return Conversion time in cycles * 2, or 0 if unsupported.
 */
static uint32_t get_conversion_time_cycles_2x(uint32_t resolution)
{
    static Lookup const conv_time_table[] = {
        { ADC_RESOLUTION_12B, 25 }, // 12.5 cycles * 2
        { ADC_RESOLUTION_10B, 21 }, // 10.5 cycles * 2
        { ADC_RESOLUTION_8B, 17 }, // 8.5 cycles * 2
        { ADC_RESOLUTION_6B, 13 }, // 6.5 cycles * 2
    };

    for (size_t i = 0; i < sizeof(conv_time_table) / sizeof(conv_time_table[0]);
         ++i) {
        if (resolution == conv_time_table[i].key) {
            return conv_time_table[i].value;
        }
    }
    return 0;
}

/**
 * @brief Gets sample time cycles for ADC sampling time setting.
 *
 * @param sample_time ADC sampling time setting.
 * @return Sample time in cycles * 2, or 0 if unsupported.
 */
static uint32_t get_sample_time_cycles_2x(uint32_t sample_time)
{
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

    for (size_t i = 0;
         i < sizeof(sample_time_table) / sizeof(sample_time_table[0]);
         ++i) {
        if (sample_time == sample_time_table[i].key) {
            return sample_time_table[i].value;
        }
    }
    return 0;
}

/**
 * @brief Gets prescaler value for ADC clock prescaler setting.
 *
 * @param clock_prescaler ADC clock prescaler setting.
 * @return Prescaler value, or 0 if unsupported.
 */
static uint32_t get_clock_prescaler_value(uint32_t clock_prescaler)
{
    static Lookup const prescaler_table[] = {
        { ADC_CLOCK_SYNC_PCLK_DIV1, 1 }, { ADC_CLOCK_SYNC_PCLK_DIV2, 2 },
        { ADC_CLOCK_SYNC_PCLK_DIV4, 4 }, { ADC_CLOCK_ASYNC_DIV1, 1 },
        { ADC_CLOCK_ASYNC_DIV2, 2 },     { ADC_CLOCK_ASYNC_DIV4, 4 },
        { ADC_CLOCK_ASYNC_DIV8, 8 },     { ADC_CLOCK_ASYNC_DIV16, 16 },
        { ADC_CLOCK_ASYNC_DIV32, 32 },   { ADC_CLOCK_ASYNC_DIV64, 64 },
        { ADC_CLOCK_ASYNC_DIV128, 128 }, { ADC_CLOCK_ASYNC_DIV256, 256 },
    };

    for (size_t i = 0; i < sizeof(prescaler_table) / sizeof(prescaler_table[0]);
         ++i) {
        if (clock_prescaler == prescaler_table[i].key) {
            return prescaler_table[i].value;
        }
    }
    return 0;
}

uint32_t ADC_LL_get_sample_rate(void)
{
    if (!g_adc_instance.initialized) {
        return 0;
    }

    uint32_t adc_clock_hz =
        PLATFORM_get_peripheral_clock_speed(PLATFORM_CLOCK_ADC1);
    if (adc_clock_hz == 0) {
        return 0;
    }

    uint32_t conv_time_cycles_2x = get_conversion_time_cycles_2x(
        g_adc_instance.adc_handles[0]->Init.Resolution
    );
    if (conv_time_cycles_2x == 0) {
        LOG_ERROR("ADC_LL_get_sample_rate: Unsupported ADC resolution");
        return 0;
    }

    uint32_t sample_time_cycles_2x =
        get_sample_time_cycles_2x(g_adc_instance.adc_configs[0].SamplingTime);
    if (sample_time_cycles_2x == 0) {
        LOG_ERROR("ADC_LL_get_sample_rate: Unsupported ADC sample time");
        return 0;
    }

    uint32_t prescaler = get_clock_prescaler_value(
        g_adc_instance.adc_handles[0]->Init.ClockPrescaler
    );
    if (prescaler == 0) {
        LOG_ERROR("ADC_LL_get_sample_rate: Unsupported ADC clock prescaler");
        return 0;
    }

    uint32_t total_cycles = (sample_time_cycles_2x + conv_time_cycles_2x) / 2;
    return adc_clock_hz / (total_cycles * prescaler);
}

uint32_t ADC_LL_get_reference_voltage(void)
{
    if (g_adc_instance.initialized) {
        return g_adc_instance.vref_mv;
    }
    return 0;
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    (void)hadc;

    if (g_adc_instance.initialized &&
        g_adc_instance.complete_callback != nullptr) {
        uint32_t total_samples = 0;

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
