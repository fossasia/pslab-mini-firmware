/**
 * @file led_ll.h
 * @brief Low-level LED control interface for PSLab Mini
 * 
 * This module provides a low-level interface to control the PSLab's onboard LEDs.
 * It offers basic initialization and toggle functionality for the LED hardware.
 * 
 * @author PSLab Team
 * @date 2025
 */

#ifndef PSLAB_LL_LED_H
#define PSLAB_LL_LED_H

/**
 * @brief Initialize the LED hardware
 * 
 * Configures the GPIO pins and enables the necessary clocks for LED control.
 * This function must be called before any other LED operations.
 */
void LED_LL_init(void);

/**
 * @brief Toggle the LED state
 * 
 * Toggles the current state of the onboard LED (on->off, off->on).
 * The LED must be initialized with LED_LL_init() before calling this function.
 */
void LED_LL_toggle(void);

#endif // PSLAB_LL_LED_H
