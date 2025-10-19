/**
 * @file led_ll.h
 * @brief Low-level LED control interface for PSLab Mini
 *
 * This module provides a low-level interface to control the PSLab's onboard
 * LEDs. It offers basic initialization and toggle functionality for the LED
 * hardware.
 *
 * @author PSLab Team
 * @date 2025
 */

#ifndef PSLAB_LL_LED_H
#define PSLAB_LL_LED_H

/**
 * @brief LED identifiers for the Nucleo-H563ZI board
 */
typedef enum {
    LED_LL_GREEN = 0, /**< Green LED on PB12 */
    LED_LL_COUNT /**< Total number of LEDs */
} LED_LL_ID;

/**
 * @brief Initialize the LED hardware
 *
 * Configures the GPIO pins and enables the necessary clocks for all LED
 * control. This function must be called before any other LED operations.
 */
void LED_LL_init(void);

/**
 * @brief Turn on the specified LED
 *
 * @pre LED_LL_init() must be called before using this function.
 *
 * @param led_id The ID of the LED to turn on (LED_LL_GREEN).
 */
void LED_LL_on(LED_LL_ID led_id);

/**
 * @brief Turn off the specified LED
 *
 * @pre LED_LL_init() must be called before using this function.
 *
 * @param led_id The ID of the LED to turn off (LED_LL_GREEN).
 */
void LED_LL_off(LED_LL_ID led_id);

/**
 * @brief Toggle the specified LED state
 *
 * @pre LED_LL_init() must be called before using this function.
 *
 * @param led_id The LED to toggle (LED_LL_GREEN).
 */
void LED_LL_toggle(LED_LL_ID led_id);

#endif // PSLAB_LL_LED_H
