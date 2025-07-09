/**
 * @file tim_ll.h
 * @brief Low-level timer functions for the PSLab Mini firmware.
 *
 * This header file defines the low-level interface for timer operations,
 * including initialization, configuration, and control of hardware timers.
 * It is intended to be used by the hardware abstraction layer (HAL) and
 * other system components that require direct access to timer functionalities.
 *
 * @author Tejas Garg
 * @date 2025-07-07
 */

#ifndef PSLAB_TIM_LL_H
#define PSLAB_TIM_LL_H

#include <stdint.h>

/*
 * @brief TIM instance enumeration
 */
typedef enum { TIM_NUM_0 = 0, TIM_NUM_1 = 1, TIM_NUM_COUNT = 2 } TIM_Num;

/**
 * @brief Start the Timer Module
 *
 * @param tim TIM instance instance
 * @param freq Frequency for the timer
 */
void TIM_LL_Start(TIM_Num tim, uint32_t freq);

/**
 * @brief Stop the Timer Module
 *
 * @param tim TIM instance instance
 */
void TIM_LL_Stop(TIM_Num tim);

#endif // PSLAB_TIM_LL_H