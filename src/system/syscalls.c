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
 * The UART bus used for I/O can be configured via the SYSCALLS_UART_BUS
 * preprocessor macro:
 * - Define SYSCALLS_UART_BUS to specify the UART bus number (0, 1, 2, etc.)
 * - Define SYSCALLS_UART_BUS as -1 or leave undefined to disable UART I/O
 *   (functions will be no-ops returning error codes)
 *
 * Buffer size for TX can be configured via:
 * - SYSCALLS_UART_TX_BUFFER_SIZE (default: 256)
 *
 * Note: RX functionality is not implemented as this is designed for
 * write-only logging and debugging output. A minimal 1-byte RX buffer
 * is allocated to satisfy the UART driver requirements, but reads will
 * always return ENOSYS.
 *
 * Example configuration:
 * #define SYSCALLS_UART_BUS 0
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

#ifndef SYSCALLS_UART_TX_BUFFER_SIZE
#define SYSCALLS_UART_TX_BUFFER_SIZE 256
#endif

// Minimal RX buffer (required by UART driver)
#define SYSCALLS_UART_RX_BUFFER_SIZE 1

// Static buffers and handle for UART I/O
static UART_Handle *g_uart_handle = nullptr;
static BUS_CircBuffer g_rx_buffer, g_tx_buffer;
static uint8_t g_rx_data[SYSCALLS_UART_RX_BUFFER_SIZE];
static uint8_t g_tx_data[SYSCALLS_UART_TX_BUFFER_SIZE];
static bool g_uart_initialized = false;

// Set to tell UART driver that syscalls is claiming the UART bus
bool g_SYSCALLS_uart_claim = false;

// Set to tell UART driver that syscalls is claiming the UART bus
bool g_SYSCALLS_uart_claim = false;

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
 * @brief Initialize UART for syscalls (called automatically on first use)
 */
static void syscalls_uart_init(void)
{
    if (g_uart_initialized) {
        return;
    }

    circular_buffer_init(&g_rx_buffer, g_rx_data, SYSCALLS_UART_RX_BUFFER_SIZE);
    circular_buffer_init(&g_tx_buffer, g_tx_data, SYSCALLS_UART_TX_BUFFER_SIZE);

    g_SYSCALLS_uart_claim = true; // Claim UART for syscalls
    g_uart_handle = UART_init(SYSCALLS_UART_BUS, &g_rx_buffer, &g_tx_buffer);
    g_SYSCALLS_uart_claim = false; // Prevent further claims
    g_uart_initialized = true;
}

/**
 * @brief Deinitialize UART for syscalls
 */
void syscalls_uart_deinit(void)
{
    if (g_uart_handle != nullptr) {
        UART_deinit(g_uart_handle);
        g_uart_handle = nullptr;
    }
    g_uart_initialized = false;
}

#endif /* SYSCALLS_UART_BUS >= 0 */

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

#if SYSCALLS_UART_BUS >= 0
    if (fd == STDOUT_FILENO || fd == STDERR_FILENO) {
        if (!g_uart_initialized) {
            syscalls_uart_init();
        }

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
#else
    (void)fd;
#endif

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
