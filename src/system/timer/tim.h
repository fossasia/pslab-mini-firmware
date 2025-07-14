/**
 * @file tim.h
 * @brief Timer module for the PSLab Mini firmware.
 *
 * This header file defines the interface for the timer module, which provides
 * functions to initialize and manage timers for various purposes, including PWM
 * generation, delay functions, and periodic interrupts.
 *
 * @author Tejas Garg
 * @date 2025-07-07
 */

#ifndef TIM_H
#define TIM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Timer handle structure
 */
typedef struct TIM_Handle TIM_Handle;

/**
 * @brief Initialize the Timer Module
 *
 * This function initializes the specified TIM instance with the given frequency
 * and starts it. It allocates a handle for the timer and returns it.
 *
 * @param tim TIM instance to start
 * @param freq Frequency for the timer
 *
 * @return timer handle
 */
TIM_Handle *TIM_init(size_t tim, uint32_t freq);

/**
 * @brief Start the Timer Module
 *
 * This function starts the specified TIM instance with the given frequency.
 * It must be called after TIM_init to start the timer.
 *
 * @param tim TIM instance to start
 * @param freq Frequency for the timer
 */
void TIM_start(size_t tim, uint32_t freq);

/**
 * @brief Stop the Timer Module
 *
 * This function stops the specified TIM instance and deinitializes it.
 * It frees the allocated handle for the timer.
 *
 * @param handle Pointer to the TIM handle structure
 */
void TIM_stop(size_t tim);

#ifdef __cplusplus
}
#endif

#endif // TIM_H