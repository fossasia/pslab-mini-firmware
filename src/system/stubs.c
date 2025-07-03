/**
 * @file stubs.c
 * @brief System call stubs for newlib
 *
 * This file provides stub implementations for system calls that newlib
 * requires. These stubs are minimal implementations that return error codes,
 * allowing the program to compile and link successfully.
 *
 * To enable stdio.h functionality (printf, scanf, etc.), proper implementations
 * of these system calls can be added:
 * - _write_r: Implement to redirect output to UART, USB, or other interface
 * - _read_r: Implement to read input from UART, USB, or other interface
 * - _close_r, _lseek_r: Can remain as stubs for basic stdio functionality
 *
 * Example implementation for UART output:
 * _ssize_t _write_r(struct _reent *r, int fd, const void *buf, size_t cnt) {
 *     if (fd == STDOUT_FILENO || fd == STDERR_FILENO) {
 *         // Send data via UART
 *         cnt = UART_write((uint8_t *)buf, cnt);
 *         return cnt;
 *     }
 *     errno = EBADF;
 *     return -1;
 * }
 */

#include <errno.h>
#include <reent.h>
#include <unistd.h>

int _close_r(struct _reent *r, int fd)
{
    (void)r;
    (void)fd;
    errno = ENOSYS;
    return -1;
}

_off_t _lseek_r(struct _reent *r, int fd, _off_t offset, int whence)
{
    (void)r;
    (void)fd;
    (void)offset;
    (void)whence;
    errno = ENOSYS;
    return (_off_t)-1;
}

_ssize_t _read_r(struct _reent *r, int fd, void *buf, size_t cnt)
{
    (void)r;
    (void)fd;
    (void)buf;
    (void)cnt;
    errno = ENOSYS;
    return -1;
}

_ssize_t _write_r(struct _reent *r, int fd, void const *buf, size_t cnt)
{
    (void)r;
    (void)fd;
    (void)buf;
    (void)cnt;
    errno = ENOSYS;
    return -1;
}
