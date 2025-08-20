/**
 * @file CExceptionConfig.h
 * @brief CException library configuration for PSLab firmware
 *
 * This file configures the CException library for use in PSLab firmware.
 * It provides a configurable uncaught exception handler mechanism that
 * allows the hardware-agnostic util library to delegate error handling
 * to platform-specific code.
 */

#ifndef CEXCEPTION_CONFIG_H
#define CEXCEPTION_CONFIG_H

#include "exception.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief CException configuration macro for uncaught exception handling
 *
 * This macro is called by the CException library when an exception is
 * thrown but not caught. It delegates to the registered handler if one
 * is set, otherwise it does nothing (which may result in undefined behavior).
 *
 * @param id The exception ID that was not caught
 */
#define CEXCEPTION_NO_CATCH_HANDLER(id)                                        \
    do {                                                                       \
        EXCEPTION_halt(id);                                                    \
        __builtin_unreachable();                                               \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif // CEXCEPTION_CONFIG_H
