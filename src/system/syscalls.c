#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/stat.h>
#include <unistd.h>

#include "system/bus/uart.h"
#include "util/error.h"

static UART_Handle *logging_uart;

void syscalls_init(UART_Handle *handle)
{
    if (logging_uart) {
        THROW(ERROR_RESOURCE_BUSY);
    }
    logging_uart = handle;
}

void syscalls_deinit(UART_Handle *handle)
{
    if (logging_uart != handle) {
        THROW(ERROR_INVALID_ARGUMENT);
    }
    logging_uart = NULL;
}

bool syscalls_uart_flush(uint32_t timeout)
{
    return logging_uart && UART_flush(logging_uart, timeout);
}

int _write(int fd, char const *buf, int count)
{
    if (!buf) {
        errno = EFAULT;
        return -1;
    }
    if (count == 0) {
        return 0;
    }
    if (fd != STDOUT_FILENO && fd != STDERR_FILENO) {
        errno = EBADF;
        return -1;
    }
    if (!logging_uart) {
        errno = EIO;
        return -1;
    }

    uint32_t written =
        UART_write(logging_uart, (uint8_t const *)buf, (uint32_t)count);
    if (written == 0 && count > 0) {
        errno = EAGAIN;
        return -1;
    }
    return (int)written;
}

int _read(int fd, char *buf, int count)
{
    (void)fd;
    if (count == 0) {
        return 0;
    }
    if (!buf) {
        errno = EFAULT;
    } else {
        errno = ENOSYS;
    }
    return -1;
}

int _fstat(int fd, struct stat *st)
{
    if (!st) {
        errno = EFAULT;
        return -1;
    }
    if (fd != STDOUT_FILENO && fd != STDERR_FILENO) {
        errno = EBADF;
        return -1;
    }

    st->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int fd)
{
    if (fd == STDOUT_FILENO || fd == STDERR_FILENO) {
        return 1;
    }
    errno = ENOTTY;
    return 0;
}
