/**
 * @file scpi_test_helpers.h
 * @brief Common test helper macros and functions for SCPI protocol testing
 *
 * This file contains reusable test helper macros and functions that are
 * commonly used across multiple SCPI protocol test files.
 *
 * @author PSLab Team
 * @date 2025-10-03
 */

#ifndef SCPI_TEST_HELPERS_H
#define SCPI_TEST_HELPERS_H

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "unity.h"
#include "mock_usb.h"
#include "application/protocol.h"

// Test buffer sizes
#define SCPI_TEST_USB_BUFFER_SIZE 512
#define SCPI_TEST_RESPONSE_BUFFER_SIZE 1024

/**
 * @brief Assert that a SCPI error has occurred
 *
 * This macro checks that the error response string does NOT start with "0,",
 * which would indicate "No error". Any other response indicates an error occurred.
 *
 * @param error_response The SCPI error response string to check
 *
 * Usage:
 *   TEST_ASSERT_SCPI_ERROR(get_captured_response());
 */
#define TEST_ASSERT_SCPI_ERROR(error_response) \
    do { \
        TEST_ASSERT_NOT_NULL_MESSAGE(error_response, "Error response is NULL"); \
        TEST_ASSERT_TRUE_MESSAGE(strlen(error_response) > 0, "Error response is empty"); \
        TEST_ASSERT_NOT_EQUAL_MESSAGE(0, strncmp(error_response, "0,", 2), \
            "Expected SCPI error but got 'No error' response"); \
    } while(0)

/**
 * @brief Assert that no SCPI error has occurred
 *
 * This macro checks that the error response string starts with "0,",
 * which indicates "No error".
 *
 * @param error_response The SCPI error response string to check
 *
 * Usage:
 *   TEST_ASSERT_SCPI_NO_ERROR(get_captured_response());
 */
#define TEST_ASSERT_SCPI_NO_ERROR(error_response) \
    do { \
        TEST_ASSERT_NOT_NULL_MESSAGE(error_response, "Error response is NULL"); \
        TEST_ASSERT_TRUE_MESSAGE(strlen(error_response) > 0, "Error response is empty"); \
        TEST_ASSERT_EQUAL_MESSAGE(0, strncmp(error_response, "0,", 2), \
            "Expected no SCPI error but got error response"); \
    } while(0)

// ============================================================================
// Global test state - each test file should define these
// ============================================================================

// These need to be defined in each test file that uses the helpers
extern char g_scpi_test_captured_response[SCPI_TEST_RESPONSE_BUFFER_SIZE];
extern size_t g_scpi_test_captured_response_len;
extern char g_scpi_test_injected_data[SCPI_TEST_USB_BUFFER_SIZE];
extern size_t g_scpi_test_injected_data_len;

// ============================================================================
// Mock USB callback functions
// ============================================================================

/**
 * @brief Mock USB_write implementation that captures response data
 */
uint32_t scpi_mock_usb_write_capture(USB_Handle *handle, uint8_t const *data, uint32_t len, int cmock_num_calls);

/**
 * @brief Mock USB_read implementation that returns injected data
 */
uint32_t scpi_mock_usb_read_inject(USB_Handle *handle, uint8_t *buffer, uint32_t max_len, int cmock_num_calls);

/**
 * @brief Mock USB_rx_ready that returns whether injected data is available
 */
bool scpi_mock_usb_rx_ready_check(USB_Handle *handle, int cmock_num_calls);

// ============================================================================
// Helper functions
// ============================================================================

/**
 * @brief Helper to inject USB command data for testing
 */
void scpi_inject_usb_command(const char *command);

/**
 * @brief Helper to get captured USB response
 */
const char *scpi_get_captured_response(void);

/**
 * @brief Helper to clear captured response
 */
void scpi_clear_captured_response(void);

/**
 * @brief Helper to run protocol task with standard USB mock expectations
 *
 * This helper sets up the common pattern of USB mock expectations and runs
 * the protocol task. Use this after injecting USB commands to advance the
 * protocol state machine.
 *
 * @param usb_handle The USB handle to use for expectations
 */
void scpi_run_protocol_with_usb_mocks(USB_Handle *usb_handle);

#endif /* SCPI_TEST_HELPERS_H */