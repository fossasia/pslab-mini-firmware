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
