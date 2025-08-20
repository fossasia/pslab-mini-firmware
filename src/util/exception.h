/**
 * @file exception.h
 * @brief Exception handler type definitions and declarations
 *
 * This header provides the exception handler types and function declarations
 * needed by both the CException configuration and the main error handling
 * system. It exists as a separate header to avoid circular dependencies.
 */

#ifndef PSLAB_EXCEPTION_H
#define PSLAB_EXCEPTION_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Last resort uncaught exception handler
 *
 * This function is called when an exception is thrown but not caught.
 * The default implementation does nothing and hangs indefinitely.
 *
 * It is recommended to provide platform-specific behavior to reset or
 * otherwise recover the system by overriding this function.
 *
 * @param exception_id The ID (error code) of the uncaught exception
 *
 * @note Overriding functions must not return.
 */
__attribute__((weak, noreturn)) void EXCEPTION_halt(uint32_t exception_id);

#ifdef __cplusplus
}
#endif

#endif // PSLAB_EXCEPTION_H
