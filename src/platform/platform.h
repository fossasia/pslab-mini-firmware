/**
 * @file platform.h
 * @brief Platform hardware initialization interface
 *
 * This file defines the primary hardware initialization interface for the
 * PSLab Mini firmware. It provides the essential platform setup functions
 * that must be called immediately after system reset to configure the
 * hardware for proper operation.
 *
 * The platform layer handles critical system initialization including:
 * - System clock configuration
 * - Core hardware initialization
 * - Power management setup
 *
 * @author PSLab Team
 * @date 2025
 */

#ifndef PSLAB_LL_PLATFORM_H
#define PSLAB_LL_PLATFORM_H

#include <stdint.h>

/**
 * @brief Initialize the platform hardware
 *
 * This function performs the primary hardware initialization for the PSLab Mini
 * system. It configures essential system components including clocks, power
 * management, and core peripherals required for proper system operation.
 *
 * This function MUST be called immediately after system reset, before any other
 * system initialization or application code. It establishes the fundamental
 * hardware configuration that all other subsystems depend upon.
 *
 * @note This function must be called before any other platform or application
 *       initialization functions.
 * @note If clock configuration fails, the system will hang to prevent damage.
 *
 * @return None
 */
void PLATFORM_init(void);

typedef enum {
    TIMER1,
    TIMER2,
    TIMER3,
    TIMER4,
    TIMER5,
    TIMER6,
    TIMER7,
    TIMER8,
    TIMER16,
    TIMER17
} PeripheralClock;

/**
 * @brief Get the clock speed for a specific peripheral clock
 *
 * This function retrieves the clock speed for the specified peripheral clock
 * type. It can be used to determine the frequency of various system clocks (for
 * now timers)
 *
 * @param clock The type of peripheral clock to query (TIM1, TIM2, etc.)
 *
 * @return The clock speed in Hz for the specified peripheral clock type,
 *         or 0 if an invalid type is provided.
 */
uint32_t get_peripheral_clock_speed(PeripheralClock clock);

#endif // PSLAB_LL_PLATFORM_H
