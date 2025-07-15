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
#include "error.h"
#include "logging.h"
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
static TIM_Handle *g_active_timers[TIM_NUM_COUNT] = { nullptr };

/**
 * @brief Start the Timer Module
 *
 * This function initializes the specified TIM instance with the given frequency
 * and starts it.
 *
 * @param tim TIM instance to start
 * @param freq The frequency of the timer
 *
 * @return timer handle
 */

TIM_Handle *TIM_init(size_t tim, uint32_t freq)
{
    if (tim >= TIM_NUM_COUNT) {
        THROW(ERROR_INVALID_ARGUMENT);
        return nullptr; // Invalid timer
    }

    if (g_active_timers[tim] != nullptr) {
        THROW(ERROR_RESOURCE_BUSY);
        return nullptr; // timer already started
    }

    TIM_Handle *handle = malloc(sizeof(TIM_Handle));
    if (handle == nullptr) {
        return nullptr; // Memory allocation failed
    }

    handle->tim_id = tim;
    handle->freq = freq;
    handle->initialised = true;

    g_active_timers[tim] = handle;

    TIM_LL_init((TIM_Num)tim, freq);
    return handle;
}

/**
 * @brief Start the Timer Module
 *
 * This function starts the specified TIM instance.
 *
 * @param tim TIM instance to start
 * @param freq Frequency to set for the timer
 */
void TIM_start(size_t tim)
{
    if (tim >= TIM_NUM_COUNT) {
        THROW(ERROR_INVALID_ARGUMENT);
        return;
    }

    if (g_active_timers[tim] == nullptr) {
        THROW(ERROR_INVALID_ARGUMENT);
        return;
    }

    TIM_LL_start((TIM_Num)tim);
    LOG_INFO("Starting TIM instance");
}

/**
 * @brief Stop the Timer Module
 *
 * This function stops the specified TIM instance and deinitializes it.
 *
 * @param handle Pointer to the TIM handle structure
 */
void TIM_stop(size_t tim)
{
    if (tim >= TIM_NUM_COUNT) {
        THROW(ERROR_INVALID_ARGUMENT);
        return;
    }

    if (g_active_timers[tim] == nullptr) {
        return; // Invalid timer or not started
    }

    TIM_LL_stop((TIM_Num)tim);
    g_active_timers[tim]->initialised = false;
    g_active_timers[tim]->freq = 0;
    free(g_active_timers[tim]);
    g_active_timers[tim] = nullptr;
}