/**
 * @file error.h
 * @brief Error handling utilities for PSLab firmware
 *
 * This header defines error codes and utility functions for error handling
 * in the PSLab firmware. It wraps the CException library for structured
 * exception handling and provides integration with standard errno codes.
 */

#ifndef PSLAB_ERROR_H
#define PSLAB_ERROR_H

#include "CException.h"
#include <errno.h>
#include <stdint.h>

#define TRY Try
#define CATCH Catch
#define THROW Throw

/**
 * @brief PSLab-specific error codes
 *
 * These codes are used for domain-specific PSLab functionality.
 * For system-level operations, prefer using standard errno codes.
 */
typedef enum {
    ERROR_NONE,
    ERROR_INVALID_ARGUMENT,
    ERROR_OUT_OF_MEMORY,
    ERROR_TIMEOUT,
    ERROR_RESOURCE_BUSY,
    ERROR_RESOURCE_UNAVAILABLE,
    ERROR_HARDWARE_FAULT,
    ERROR_CALIBRATION_FAILED,
    ERROR_DEVICE_NOT_READY,
    ERROR_UNKNOWN
} Error;

static inline char const *error_to_string(Error error)
{
    switch (error) {
    case ERROR_NONE:
        return "No error";
    case ERROR_INVALID_ARGUMENT:
        return "Invalid argument";
    case ERROR_OUT_OF_MEMORY:
        return "Out of memory";
    case ERROR_TIMEOUT:
        return "Operation timed out";
    case ERROR_RESOURCE_BUSY:
        return "Resource busy";
    case ERROR_RESOURCE_UNAVAILABLE:
        return "Resource unavailable";
    case ERROR_HARDWARE_FAULT:
        return "Hardware fault";
    case ERROR_CALIBRATION_FAILED:
        return "Calibration failed";
    case ERROR_DEVICE_NOT_READY:
        return "Device not ready";
    case ERROR_UNKNOWN:
    default:
        return "Unknown error";
    }
}

/**
 * @brief Convert PSLab error to appropriate errno code
 *
 * @param error PSLab error code
 * @return Corresponding errno value, or 0 for ERROR_NONE
 */
static inline int error_to_errno(Error error)
{
    switch (error) {
    case ERROR_NONE:
        return 0;
    case ERROR_INVALID_ARGUMENT:
        return EINVAL;
    case ERROR_OUT_OF_MEMORY:
        return ENOMEM;
    case ERROR_TIMEOUT:
        return ETIMEDOUT;
    case ERROR_RESOURCE_BUSY:
        return EBUSY;
    case ERROR_RESOURCE_UNAVAILABLE:
    case ERROR_HARDWARE_FAULT:
    case ERROR_CALIBRATION_FAILED:
        return EIO;
    case ERROR_DEVICE_NOT_READY:
        return EAGAIN;
    case ERROR_UNKNOWN:
    default:
        return EIO;
    }
}

/**
 * @brief Convert errno to PSLab error code (best effort)
 *
 * @param errno_val errno value
 * @return Corresponding PSLab error code
 */
static inline Error errno_to_error(int errno_val)
{
    switch (errno_val) {
    case 0:
        return ERROR_NONE;
    case EINVAL:
    case EDOM:
    case ERANGE:
        return ERROR_INVALID_ARGUMENT;
    case ENOMEM:
        return ERROR_OUT_OF_MEMORY;
    case ETIMEDOUT:
        return ERROR_TIMEOUT;
    case EIO:
    case ENODEV:
    case ENXIO:
        return ERROR_HARDWARE_FAULT;
    case EAGAIN:
#if EAGAIN != EWOULDBLOCK
    case EWOULDBLOCK:
#endif
        return ERROR_DEVICE_NOT_READY;
    case EBUSY:
        return ERROR_RESOURCE_BUSY;
    default:
        return ERROR_UNKNOWN;
    }
}

#endif // PSLAB_ERROR_H
