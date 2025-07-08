#ifndef TEST_REENT_H
#define TEST_REENT_H

/**
 * @file reent.h
 * @brief Test replacement for newlib's reent.h
 * 
 * This file provides minimal replacements for newlib's reent functionality
 * to enable testing of syscalls without requiring the full newlib environment.
 */

typedef struct _reent {
    int _errno;
} _reent;

extern _reent *__impure_ptr;

typedef long _ssize_t;
typedef long _off_t;

#endif /* TEST_REENT_H */
