/**
 * @file system.c
 * @brief Hardware system initialization routines for the PSLab Mini firmware.
 *
 * This module provides the SYSTEM_init() function, which initializes all core hardware
 * peripherals and must be called immediately after reset, before any other hardware access.
 */

#include "led.h"
#include "platform.h"

#include "system.h"

void SYSTEM_init(void)
{
    PLATFORM_init();
    LED_init();
}
