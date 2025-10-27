#ifndef TEST_ERRNO_H
#define TEST_ERRNO_H

/**
 * @file errno.h
 * @brief Test replacement for system errno.h
 *
 * This file provides minimal error code definitions for testing.
 */

// Error codes
#define EBADF       9    // Bad file descriptor
#define EIO         5    // I/O error
#define ENOSYS      38   // Function not implemented
#define EAGAIN      11   // Resource temporarily unavailable
#define EFAULT      14   // Bad address
#define ENOTTY      25   // Not a terminal
#define EINVAL      22   // Invalid argument
#define ENOMEM      12   // Out of memory
#define ETIMEDOUT   110  // Connection timed out
#define EBUSY       16   // Device or resource busy
#define EDOM        33   // Math argument out of domain
#define ERANGE      34   // Math result not representable
#define ENODEV      19   // No such device
#define ENXIO       6    // No such device or address
#define EWOULDBLOCK EAGAIN // Operation would block

// Forward errno to our fake reent
#define errno (__impure_ptr->_errno)

#endif /* TEST_ERRNO_H */
