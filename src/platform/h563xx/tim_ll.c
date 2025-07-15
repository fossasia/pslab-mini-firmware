/**
 * @file tim_ll.c
 * @brief Low-level timer functions for the PSLab Mini firmware.
 *
 * This module provides the low-level interface for timer operations,
 * including initialization, configuration, and control of hardware timers.
 * It is intended to be used by the hardware abstraction layer (HAL) and
 * other system components that require direct access to timer functionalities.
 *
 * @author Tejas Garg
 * @date 2025-07-07
 */

#include "stm32h5xx_hal.h"

#include "error.h"
#include "logging.h"
#include "platform.h"
#include "tim_ll.h"
#include <stdbool.h>

enum { TIMER_DEFAULT_PRESCALER = 0 }; // Default prescaler value
// 0 prescaler divides the timer clock by 1

/*Timer Instance Structure with parameters for any give Instance*/
typedef struct {
    TIM_HandleTypeDef *htim;
    uint32_t frequency;
    uint32_t period;
    bool initialized;
} TimerInstance;

/*TImer Handle initialization*/
static TIM_HandleTypeDef g_htim6 = { nullptr };
static TIM_HandleTypeDef g_htim7 = { nullptr };

/*Array of Timer Instances*/
static TimerInstance g_timer_instances[TIM_NUM_COUNT] = {
    [TIM_NUM_0] = {
        .htim = &g_htim6,
        .initialized = false,
    },
    [TIM_NUM_1] = {
        .htim = &g_htim7,
        .initialized = false,
    },
};

/**
 * @brief Get the clock frequency for the specified timer instance.
 *
 * This function retrieves the clock frequency for the given timer instance
 * based on its hardware configuration. It is used to calculate timer values
 * for various operations.
 *
 * @param tim The timer count of the timer
 * @return The clock frequency in Hz, or 0 if the timer instance is invalid.
 */
static uint32_t get_timer_clock_frequency(TIM_Num tim)
{

    TimerInstance *instance = &g_timer_instances[tim];

    PLATFORM_PeripheralClock clock_type = PLATFORM_CLOCK_INVALID;

    if (instance->htim->Instance == TIM1) {
        clock_type = PLATFORM_CLOCK_TIMER1;
    }
    if (instance->htim->Instance == TIM2) {
        clock_type = PLATFORM_CLOCK_TIMER2;
    }
    if (instance->htim->Instance == TIM3) {
        clock_type = PLATFORM_CLOCK_TIMER3;
    }
    if (instance->htim->Instance == TIM4) {
        clock_type = PLATFORM_CLOCK_TIMER4;
    }
    if (instance->htim->Instance == TIM5) {
        clock_type = PLATFORM_CLOCK_TIMER5;
    }
    if (instance->htim->Instance == TIM6) {
        clock_type = PLATFORM_CLOCK_TIMER6;
    }
    if (instance->htim->Instance == TIM7) {
        clock_type = PLATFORM_CLOCK_TIMER7;
    }
    if (instance->htim->Instance == TIM8) {
        clock_type = PLATFORM_CLOCK_TIMER8;
    }
    if (instance->htim->Instance == TIM16) {
        clock_type = PLATFORM_CLOCK_TIMER16;
    }
    if (instance->htim->Instance == TIM17) {
        clock_type = PLATFORM_CLOCK_TIMER17;
    }

    return PLATFORM_get_peripheral_clock_speed(clock_type
    ); // Return the system clock frequency
}

/**
 * @brief Calculate timer values based on the specified period.
 *
 * This function calculates the timer values required to achieve a specific
 * period for the timer. It uses the timer clock frequency to determine the
 * appropriate prescaler and auto-reload values.
 *
 * @param tim The timer count of the timer
 */
static void calculate_timer_values(TIM_Num tim)
{
    TimerInstance *instance = &g_timer_instances[tim];
    if (instance->frequency == 0) {
        // frequency should not be zero
        THROW(ERROR_INVALID_ARGUMENT);
        return;
    }

    uint32_t tim_clock = get_timer_clock_frequency(tim);

    if (tim_clock == 0) {
        // Invalid timer clock frequency
        THROW(ERROR_INVALID_ARGUMENT);
        return;
    }

    instance->period = (tim_clock / instance->frequency) - 1;
}

/**
 * @brief Initialize the timer base MSP (MCU Support Package).
 *
 * This function is called by the HAL when initializing the timer base.
 * It enables the clock for the specified timer instance and performs any
 * necessary hardware-specific initialization.
 *
 * @param htim Pointer to the TIM_HandleTypeDef structure that contains
 *             the configuration information for the specified timer.
 */
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM6) {
        /* Enable TIM6 clock */
        __HAL_RCC_TIM6_CLK_ENABLE();
    }

    if (htim->Instance == TIM7) {
        /* Enable TIM7 clock */
        __HAL_RCC_TIM7_CLK_ENABLE();
    }
}

/**
 * @brief Initialize TIM module
 *
 * @param tim Timer instance
 * @param freq Frequency for the timer
 */
void TIM_LL_init(TIM_Num tim, uint32_t freq)
{
    if (tim >= TIM_NUM_COUNT) {
        THROW(ERROR_INVALID_ARGUMENT);
        return;
    }

    if (g_timer_instances[tim].initialized) {
        THROW(ERROR_RESOURCE_BUSY);
        return;
    }

    TimerInstance *instance = &g_timer_instances[tim];

    instance->frequency = freq;

    if (tim == TIM_NUM_0) {
        instance->htim->Instance = TIM6;
        instance->htim->Channel = TIM_CHANNEL_1;
    } else if (tim == TIM_NUM_1) {
        instance->htim->Instance = TIM7;
        instance->htim->Channel = TIM_CHANNEL_1;
    }

    calculate_timer_values(tim);

    instance->htim->Init.Prescaler = TIMER_DEFAULT_PRESCALER;
    instance->htim->Init.CounterMode = TIM_COUNTERMODE_UP;
    instance->htim->Init.Period = instance->period;
    instance->htim->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    instance->htim->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    if (HAL_TIM_Base_Init(instance->htim) != HAL_OK) {
        THROW(ERROR_HARDWARE_FAULT);
        return;
    }

    TIM_MasterConfigTypeDef s_master_config = { 0 };

    s_master_config.MasterOutputTrigger = TIM_TRGO_UPDATE;
    s_master_config.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;

    if (HAL_TIMEx_MasterConfigSynchronization(
            instance->htim, &s_master_config
        ) != HAL_OK) {
        THROW(ERROR_HARDWARE_FAULT);
        return;
    }

    instance->initialized = true;
}

/**
 * @brief Deinitialize the TIM peripheral.
 *
 * @param tim TIM instance to deinitialize
 */
void TIM_LL_deinit(TIM_Num tim)
{
    if (tim >= TIM_NUM_COUNT) {
        THROW(ERROR_INVALID_ARGUMENT);
        return;
    }

    if (!g_timer_instances[tim].initialized) {
        return;
    }

    TimerInstance *instance = &g_timer_instances[tim];

    HAL_TIM_Base_DeInit(instance->htim);

    instance->frequency = 0;
    instance->period = 0;
    instance->initialized = false;
}

/**
 * @brief Start the Timer Module
 *
 * @param tim TIM instance instance
 * @param freq Frequency for the timer
 */
void TIM_LL_start(TIM_Num tim)
{
    TimerInstance *instance = &g_timer_instances[tim];
    if (HAL_TIM_Base_Start(instance->htim) != HAL_OK) {
        THROW(ERROR_HARDWARE_FAULT);
        return;
    }
    if (HAL_TIM_PWM_Start(instance->htim, TIM_CHANNEL_1) != HAL_OK) {
        THROW(ERROR_HARDWARE_FAULT);
        return;
    }
}

/**
 * @brief Stop the Timer Module
 *
 * @param tim TIM instance instance
 */
void TIM_LL_stop(TIM_Num tim)
{
    TimerInstance *instance = &g_timer_instances[tim];
    if (HAL_TIM_Base_Stop(instance->htim) != HAL_OK) {
        THROW(ERROR_HARDWARE_FAULT);
        return;
    }
}
