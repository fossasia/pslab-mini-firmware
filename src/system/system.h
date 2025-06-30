/**
 * @file system.h
 * @brief Hardware system initialization interface for the PSLab Mini firmware.
 *
 * This module declares the SYSTEM_init() function, which must be called immediately after reset
 * to initialize all core hardware peripherals before any other hardware access.
 */

#ifndef SYSTEM_H
#define SYSTEM_H

/**
 * @brief Initialize all core hardware peripherals.
 *
 * This function must be called immediately after reset, before any other hardware
 * access is performed. It initializes the platform, LED, UART, and USB subsystems.
 */
void SYSTEM_init(void);

#endif // SYSTEM_H
