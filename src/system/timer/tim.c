/**
 * @file tim.c
 * @brief Timer module for the PSLab Mini firmware.
 *
 * This module provides functions to initialize and manage timers for various
 * purposes, including PWM generation, delay functions, and periodic interrupts.
 *
 * @author Tejas Garg
 * @date 2025-07-07
 */

#include "tim.h"
#include "tim_ll.h"
#include <stddef.h>
#include <stdlib.h>

/**
 * @brief Timer handle structure
 */
struct TIM_Handle {
    TIM_Num tim_id;
    uint32_t freq;
    bool initialised;
};

// Array to keep track of active timers
static TIM_Handle *active_timers[TIM_NUM_COUNT] = { nullptr };

/**
 * @brief Start the Timer Module
 *
 * This function initializes the specified TIM instance with the given frequency
 * and starts it.
 *
 * @param tim TIM instance to start
 */
TIM_Handle *TIM_Init(size_t tim, uint32_t freq)
{
    if (tim >= TIM_NUM_COUNT || active_timers[tim] != nullptr) {
        return nullptr; // Invalid timer or already started
    }

    TIM_LL_Start(tim, freq);

    TIM_Handle *handle = malloc(sizeof(TIM_Handle));
    if (handle == nullptr) {
        return nullptr; // Memory allocation failed
    }

    handle->tim_id = tim;
    handle->freq = freq;
    handle->initialised = true;

    active_timers[tim] = handle;

    return handle;
}

/**
 * @brief Stop the Timer Module
 *
 * This function stops the specified TIM instance and deinitializes it.
 *
 * @param handle Pointer to the TIM handle structure
 */
void TIM_Stop(size_t tim)
{
    if (tim >= TIM_NUM_COUNT || active_timers[tim] == nullptr) {
        return; // Invalid timer or not started
    }

    TIM_LL_Stop((TIM_Num)tim);
    active_timers[tim]->initialised = false;
    active_timers[tim]->freq = 0;
    free(active_timers[tim]);
    active_timers[tim] = nullptr;
}