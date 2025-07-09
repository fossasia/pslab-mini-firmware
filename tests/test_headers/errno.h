#ifndef TEST_ERRNO_H
#define TEST_ERRNO_H

/**
 * @file errno.h
 * @brief Test replacement for system errno.h
 * 
 * This file provides minimal error code definitions for testing.
 */

// Error codes
#define EBADF   9    // Bad file descriptor
#define EIO     5    // I/O error
#define ENOSYS  38   // Function not implemented
#define EAGAIN  11   // Resource temporarily unavailable
#define EFAULT  14   // Bad address
#define ENOTTY  25   // Not a terminal

// Forward errno to our fake reent
#define errno (__impure_ptr->_errno)

#endif /* TEST_ERRNO_H */
