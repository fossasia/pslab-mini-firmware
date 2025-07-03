/**
 * @file led.h
 * @brief LED system interface - simple wrapper around the led_ll interface
 * 
 * This module provides a high-level LED interface that wraps the low-level
 * LED hardware abstraction layer (led_ll). It offers a clean, platform-independent
 * API for LED control operations.
 */

#ifndef SYSTEM_LED_H
#define SYSTEM_LED_H

/**
 * @brief Initialize the LED system
 * 
 * Initializes the LED hardware and prepares it for use. This function
 * must be called before any other LED operations.
 */
void LED_init(void);

/**
 * @brief Toggle the LED state
 * 
 * Switches the LED from its current state (on to off, or off to on).
 */
void LED_toggle(void);

#endif // SYSTEM_LED_H
