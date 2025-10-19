/**
 * @file led.h
 * @brief LED system interface
 *
 * This module provides a high-level LED interface.
 */

#ifndef SYSTEM_LED_H
#define SYSTEM_LED_H

typedef enum { LED_GREEN, LED_COUNT } LED_ID;

/**
 * @brief Initialize the LED system
 *
 * Initializes the LED hardware and prepares it for use. This function
 * must be called before any other LED operations.
 */
void LED_init(void);

/**
 * @brief Turn on the specified LED
 *
 * Activates the specified LED, turning it on.
 *
 * @param led_id The ID of the LED to turn on (LED_GREEN).
 */
void LED_on(LED_ID led_id);

/**
 * @brief Turn off the specified LED
 *
 * Deactivates the specified LED, turning it off.
 *
 * @param led_id The ID of the LED to turn off (LED_GREEN).
 */
void LED_off(LED_ID led_id);

/**
 * @brief Toggle the LED state
 *
 * Switches the LED from its current state (on to off, or off to on).
 *
 * @param led_id The ID of the LED to toggle (LED_GREEN).
 */
void LED_toggle(LED_ID led_id);

#endif // SYSTEM_LED_H
