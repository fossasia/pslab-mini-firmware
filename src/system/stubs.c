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

/**
 * @brief Reentrant version of getpid system call stub
 *
 * This stub is required by newlib's reentrant infrastructure when using
 * setjmp/longjmp (as used by CException). In bare-metal embedded systems,
 * there are no processes, so we return a fixed process ID.
 *
 * @param r Pointer to reentrant structure (unused)
 * @return Always returns 1 (fixed process ID for embedded systems)
 */
int _getpid_r(struct _reent *r)
{
    (void)r;
    return 1;
}

/**
 * @brief Reentrant version of kill system call stub
 *
 * This stub is required by newlib's reentrant infrastructure when using
 * setjmp/longjmp (as used by CException). In bare-metal embedded systems,
 * there are no processes or signals, so this always fails.
 *
 * @param r Pointer to reentrant structure (unused)
 * @param pid Process ID (unused)
 * @param sig Signal number (unused)
 * @return Always returns -1 with errno set to EINVAL
 */ // NOLINTNEXTLINE: bugprone-easily-swappable-parameters
int _kill_r(struct _reent *r, int pid, int sig)
{
    (void)r;
    (void)pid;
    (void)sig;
    errno = EINVAL;
    return -1;
}
