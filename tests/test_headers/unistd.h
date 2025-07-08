#ifndef TEST_UNISTD_H
#define TEST_UNISTD_H

/**
 * @file unistd.h
 * @brief Test replacement for system unistd.h
 * 
 * This file provides minimal POSIX definitions for testing.
 */

#include <stddef.h>

// Standard file descriptors
#define STDIN_FILENO    0   // Standard input
#define STDOUT_FILENO   1   // Standard output  
#define STDERR_FILENO   2   // Standard error

#endif /* TEST_UNISTD_H */
