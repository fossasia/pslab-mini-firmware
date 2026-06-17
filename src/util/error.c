/**
 * @file error.c
 * @brief Error handling integration for PSLab Pico firmware
 *
 * Provides the default uncaught CException halt path for the Pico build. The
 * system layer may override this weak handler once reset/logging integration is
 * wired in.
 */

#include <stdint.h>

#include "pico/stdlib.h"

__attribute__((weak, noreturn)) void EXCEPTION_halt(uint32_t id)
{
    (void)id;

    for (;;) {
        tight_loop_contents();
    }
}
