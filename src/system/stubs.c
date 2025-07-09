/**
 * @file stubs.c
 * @brief System call stubs for newlib
 *
 * This file provides stub implementations for system calls that newlib
 * requires. These stubs are minimal implementations that return error codes,
 * allowing the program to compile and link successfully.
 *
 * Note: _read_r and _write_r are implemented in syscalls.c to provide
 * actual UART-based I/O functionality for stdio.h (printf, scanf, etc.).
 * The remaining stubs (_close_r, _lseek_r) can remain as stubs for basic
 * stdio functionality.
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
