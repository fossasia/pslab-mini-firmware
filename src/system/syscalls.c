/**
 * @file syscalls.c
 * @brief System call implementations for newlib using UART
 *
 * This file provides implementations for system calls that newlib requires
 * for stdio functionality, using the UART API for I/O operations.
 *
 * Implemented syscalls:
 * - _read_r: Read from stdin via UART
 * - _write_r: Write to stdout/stderr via UART
 * - _fstat_r: File status (stub - identifies stdin/stdout/stderr as character
 *   devices)
 * - _isatty_r: Terminal check (stub - treats stdin/stdout/stderr as terminals)
 *
 * The UART bus used for I/O can be configured via the SYSCALLS_UART_BUS
 * preprocessor macro:
 * - Define SYSCALLS_UART_BUS to specify the UART bus number (0, 1, 2, etc.)
 * - Define SYSCALLS_UART_BUS as -1 or leave undefined to disable UART I/O
 *   (functions will be no-ops returning error codes)
 *
 * Buffer sizes for RX/TX can be configured via:
 * - SYSCALLS_UART_RX_BUFFER_SIZE (default: 256)
 * - SYSCALLS_UART_TX_BUFFER_SIZE (default: 256)
 *
 * Example configuration:
 * #define SYSCALLS_UART_BUS 0
 * #define SYSCALLS_UART_RX_BUFFER_SIZE 512
 * #define SYSCALLS_UART_TX_BUFFER_SIZE 512
 */

#include <errno.h>
#include <reent.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "syscalls_config.h"

// Only include UART headers if UART I/O is enabled
#ifndef SYSCALLS_UART_BUS
#define SYSCALLS_UART_BUS -1 /* Disabled by default */
#endif

#if SYSCALLS_UART_BUS >= 0
#include "bus/bus.h"
#include "bus/uart.h"

#ifndef SYSCALLS_UART_RX_BUFFER_SIZE
#define SYSCALLS_UART_RX_BUFFER_SIZE 256
#endif

#ifndef SYSCALLS_UART_TX_BUFFER_SIZE
#define SYSCALLS_UART_TX_BUFFER_SIZE 256
#endif

// Static buffers and handle for UART I/O
static UART_Handle *uart_handle = nullptr;
static BUS_CircBuffer rx_buffer, tx_buffer;
static uint8_t rx_data[SYSCALLS_UART_RX_BUFFER_SIZE];
static uint8_t tx_data[SYSCALLS_UART_TX_BUFFER_SIZE];
static bool uart_initialized = false;

static int check_args(void const *buf, size_t cnt)
{
    if (buf == nullptr) {
        errno = EFAULT;
        return -1;
    }

    // POSIX: reading 0 bytes should return 0 immediately
    if (cnt == 0) {
        return 0;
    }

    return 1;
}

/**
 * @brief Initialize UART for syscalls (called automatically on first use)
 */
static void syscalls_uart_init(void)
{
    if (uart_initialized) {
        return;
    }

    circular_buffer_init(&rx_buffer, rx_data, SYSCALLS_UART_RX_BUFFER_SIZE);
    circular_buffer_init(&tx_buffer, tx_data, SYSCALLS_UART_TX_BUFFER_SIZE);

    uart_handle = UART_init(SYSCALLS_UART_BUS, &rx_buffer, &tx_buffer);
    uart_initialized = true;
}

/**
 * @brief Deinitialize UART for syscalls
 */
void syscalls_uart_deinit(void)
{
    if (uart_handle != nullptr) {
        UART_deinit(uart_handle);
        uart_handle = nullptr;
    }
    uart_initialized = false;
}

#endif /* SYSCALLS_UART_BUS >= 0 */

/**
 * @brief Read data from file descriptor
 *
 * For stdin (fd 0), reads from UART if enabled, otherwise returns error.
 * For other file descriptors, returns error.
 */
_ssize_t _read_r(struct _reent *r, int fd, void *buf, size_t cnt)
{
    (void)r;

    // Check for invalid parameters
    int ret = check_args(buf, cnt);
    if (ret < 1) {
        return ret;
    }

#if SYSCALLS_UART_BUS >= 0
    if (fd == STDIN_FILENO) {
        if (!uart_initialized) {
            syscalls_uart_init();
        }

        if (uart_handle == nullptr) {
            errno = EIO;
            return -1;
        }

        // Non-blocking read from UART
        uint32_t bytes_read = UART_read(uart_handle, (uint8_t *)buf, cnt);
        return (_ssize_t)bytes_read;
    }
#else
    (void)fd;
#endif

    errno = EBADF;
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
    (void)r;

    // Check for invalid parameters
    int ret = check_args(buf, cnt);
    if (ret < 1) {
        return ret;
    }

#if SYSCALLS_UART_BUS >= 0
    if (fd == STDOUT_FILENO || fd == STDERR_FILENO) {
        if (!uart_initialized) {
            syscalls_uart_init();
        }

        if (uart_handle == nullptr) {
            errno = EIO;
            return -1;
        }

        // Write to UART
        uint32_t bytes_written =
            UART_write(uart_handle, (uint8_t const *)buf, cnt);

        // If no bytes were written, it likely means the buffer is full
        if (bytes_written == 0 && cnt > 0) {
            errno = EAGAIN;
            return -1;
        }

        return (_ssize_t)bytes_written;
    }
#else
    (void)fd;
#endif

    errno = EBADF;
    return -1;
}

/**
 * @brief Get file status (stub implementation)
 *
 * This is a stub implementation that identifies stdin/stdout/stderr as
 * character devices and returns an error for other file descriptors.
 */
int _fstat_r(struct _reent *r, int fd, struct stat *st)
{
    (void)r;

    if (st == nullptr) {
        errno = EFAULT;
        return -1;
    }

    // Clear the stat structure
    memset(st, 0, sizeof(struct stat));

    // For stdin, stdout, stderr - treat as character device
    if (fd == STDIN_FILENO || fd == STDOUT_FILENO || fd == STDERR_FILENO) {
        st->st_mode = S_IFCHR; // Character device
        st->st_size = 0;
        return 0;
    }

    // For other file descriptors, return error
    errno = EBADF;
    return -1;
}

/**
 * @brief Check if file descriptor is a terminal (stub implementation)
 *
 * This stub implementation returns 1 (true) for stdin/stdout/stderr
 * and 0 (false) for other file descriptors.
 */
int _isatty_r(struct _reent *r, int fd)
{
    (void)r;

    // stdin, stdout, stderr are treated as terminals
    if (fd == STDIN_FILENO || fd == STDOUT_FILENO || fd == STDERR_FILENO) {
        return 1;
    }

    // Other file descriptors are not terminals
    errno = ENOTTY;
    return 0;
}
