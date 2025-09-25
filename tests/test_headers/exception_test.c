/**
 * @file exception_test.c
 * @brief Test implementation of EXCEPTION_halt for unit tests
 *
 * This file provides a test-specific implementation of EXCEPTION_halt that
 * fails the test if an uncaught exception occurs. This should never happen
 * during tests, so reaching this function indicates a test failure.
 */

#include <stdint.h>
#include <stdio.h>
#include "unity.h"

/**
 * @brief Test implementation of exception halt function
 *
 * This function is called when a THROW occurs outside of any TRY context.
 * In tests, this should never happen, so we fail the test immediately.
 *
 * @param id The exception ID that was thrown but not caught
 *
 * @note This function does not return; it fails the current test
 */
__attribute__((noreturn)) void EXCEPTION_halt(uint32_t id)
{
    // Format a failure message with the exception ID
    char msg[64];
    snprintf(msg, sizeof(msg), "Uncaught exception: 0x%08X", (unsigned int)id);

    // Fail the test with the formatted message
    TEST_FAIL_MESSAGE(msg);

    // This should never be reached due to TEST_FAIL_MESSAGE behavior,
    // but added for completeness to satisfy the noreturn attribute
    __builtin_unreachable();
}