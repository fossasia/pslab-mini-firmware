/**
 * @file platform.c
 * @brief Pico platform hardware initialization implementation
 *
 * This file implements the platform abstraction required by reusable system
 * modules ported from the STM32 firmware.
 */

#include <stdint.h>

#include "hardware/clocks.h"
#include "hardware/watchdog.h"
#include "pico/stdlib.h"

#include "platform.h"

void PLATFORM_init(void)
{
    /* Pico SDK runtime performs the core clock/runtime setup before main(). */
}

uint32_t PLATFORM_get_tick(void)
{
    return to_ms_since_boot(get_absolute_time());
}

uint32_t PLATFORM_get_peripheral_clock_speed(PLATFORM_PeripheralClock clock)
{
    if (clock == PLATFORM_CLOCK_INVALID) {
        return 0;
    }

    return clock_get_hz(clk_sys);
}

void PLATFORM_idle(void)
{
    tight_loop_contents();
}

__attribute__((noreturn)) void PLATFORM_reset(void)
{
    watchdog_reboot(0, 0, 0);

    for (;;) {
        tight_loop_contents();
    }
}
