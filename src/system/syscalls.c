/**
 * @file syscalls.c
 * @brief System call implementations for newlib using UART (write-only)
 *
 * This file provides implementations for system calls that newlib requires
 * for stdio functionality, using the UART API for write-only I/O operations.
 * Writes are non-blocking.
 *
 * Implemented syscalls:
 * - _read_r: Stub that returns ENOSYS (reads not supported)
 * - _write_r: Write to stdout/stderr via UART
 * - _fstat_r: File status (stub - identifies stdout/stderr as character
 *   devices, stdin as invalid)
 * - _isatty_r: Terminal check (stub - treats stdout/stderr as terminals,
 *   stdin as not a terminal)
 *
 * Usage:
 * Call syscalls_init() with an initialized UART_Handle pointer to enable
 * stdout/stderr output. Until syscalls_init() is called, _write_r will
 * return EBADF.
 *
 * Note: RX functionality is not implemented as this is designed for
 * write-only logging and debugging output. Reads will always return ENOSYS.
 */

#include <errno.h>
#include <reent.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "util/error.h"
#include "util/util.h"

// Include UART headers for handle type
#include "system/bus/uart.h"

// Static handle for UART I/O
static UART_Handle *g_uart_handle = nullptr;

static int check_args(struct _reent *r, void const *buf, size_t cnt)
{
    if (buf == nullptr) {
        r->_errno = EFAULT;
        return -1;
    }

    // POSIX: reading 0 bytes should return 0 immediately
    if (cnt == 0) {
        return 0;
    }

    return 1;
}

/**
 * @brief Initialize syscalls with a UART handle
 *
 * @param handle Pointer to an initialized UART handle to use for stdout/stderr
 * @throws ERROR_RESOURCE_BUSY if syscalls is already initialized
 */
void syscalls_init(UART_Handle *handle)
{
    if (g_uart_handle != nullptr) {
        THROW(ERROR_RESOURCE_BUSY);
    }
    g_uart_handle = handle;
}

/**
 * @brief Deinitialize syscalls
 *
 * Clears the UART handle, disabling stdout/stderr output.
 *
 * @param handle Pointer to the UART handle that was used to initialize syscalls
 * @throws ERROR_INVALID_ARGUMENT if handle doesn't match the initialized handle
 */
void syscalls_deinit(UART_Handle *handle)
{
    if (g_uart_handle != handle) {
        THROW(ERROR_INVALID_ARGUMENT);
    }
    g_uart_handle = nullptr;
}

bool syscalls_uart_flush(uint32_t timeout)
{
    if (g_uart_handle == nullptr) {
        return false;
    }

    // Wait for the UART transmit buffer to be empty
    return UART_flush(g_uart_handle, timeout);
}

/**
 * @brief Read data from file descriptor (stub - reads not supported)
 *
 * Since UART is used only for write-only logging/debugging,
 * reading is not implemented. However, 0-length reads always succeed
 * per POSIX semantics.
 */
_ssize_t _read_r(struct _reent *r, int fd, void *buf, size_t cnt)
{
    (void)fd;

    // POSIX: reading 0 bytes should always succeed
    if (cnt == 0) {
        return 0;
    }

    // Check for null buffer
    if (buf == nullptr) {
        r->_errno = EFAULT;
        return -1;
    }

    r->_errno = ENOSYS; // Function not implemented
    return -1;
}

/**
 * @brief Write data to file descriptor
 *
 * For stdout/stderr (fd 1/2), writes to UART if enabled, otherwise returns
 * error. For other file descriptors, returns error.
 */
_ssize_t _write_r(struct _reent *r, int fd, void const *buf, size_t cnt)
{
    // Check for invalid parameters
    int ret = check_args(r, buf, cnt);
    if (ret < 1) {
        return ret;
    }

    if (fd == STDOUT_FILENO || fd == STDERR_FILENO) {
        if (g_uart_handle == nullptr) {
            r->_errno = EIO;
            return -1;
        }

        // Write to UART
        uint32_t bytes_written =
            UART_write(g_uart_handle, (uint8_t const *)buf, cnt);

        // If no bytes were written, it likely means the buffer is full
        if (bytes_written == 0 && cnt > 0) {
            r->_errno = EAGAIN;
            return -1;
        }

        return (_ssize_t)bytes_written;
    }

    r->_errno = EBADF;
    return -1;
}

/**
 * @brief Get file status (stub implementation)
 *
 * Since stdin reads are not supported, only stdout/stderr are treated as
 * character devices. stdin returns an error.
 */
int _fstat_r(struct _reent *r, int fd, struct stat *st)
{
    if (st == nullptr) {
        r->_errno = EFAULT;
        return -1;
    }

    // Clear the stat structure
    memset(st, 0, sizeof(struct stat));

    // Only stdout and stderr are valid - stdin reads not supported
    if (fd == STDOUT_FILENO || fd == STDERR_FILENO) {
        st->st_mode = S_IFCHR; // Character device
        st->st_size = 0;
        return 0;
    }

    // stdin and other file descriptors return error
    r->_errno = EBADF;
    return -1;
}

/**
 * @brief Check if file descriptor is a terminal (stub implementation)
 *
 * Since stdin reads are not supported, only stdout/stderr are treated as
 * terminals. stdin returns 0 (not a terminal).
 */
int _isatty_r(struct _reent *r, int fd)
{
    // Only stdout and stderr are treated as terminals
    if (fd == STDOUT_FILENO || fd == STDERR_FILENO) {
        return 1;
    }

    // stdin (not readable) and other file descriptors are not terminals
    r->_errno = ENOTTY;
    return 0;
}
