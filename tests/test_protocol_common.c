/**
 * @file test_protocol_common.c
 * @brief Unit tests for common SCPI protocol functionality
 *
 * This file contains unit tests for the common protocol functionality covering:
 * - Protocol lifecycle management (init/deinit/task)
 * - IEEE 488.2 standard SCPI commands (*IDN?, *RST, *TST?, SYST:ERR?)
 * - USB communication handling and error recovery
 * - State management and reset functionality
 *
 * The protocol module uses USB CDC as transport and implements SCPI commands
 * for instrument control.
 *
 * @author PSLab Team
 * @date 2025-10-02
 */

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "unity.h"
#include "mock_dmm.h"
#include "mock_dso.h"
#include "mock_usb.h"
#include "mock_system.h"
#include "scpi_test_helpers.h"

#include "util/error.h"
#include "util/fixed_point.h"
#include "util/util.h"

#include "application/protocol.h"

// Define global variables required by scpi_test_helpers
char g_scpi_test_captured_response[SCPI_TEST_RESPONSE_BUFFER_SIZE];
size_t g_scpi_test_captured_response_len;
char g_scpi_test_injected_data[SCPI_TEST_USB_BUFFER_SIZE];
size_t g_scpi_test_injected_data_len;

// Test fixtures and mock data
static USB_Handle *g_mock_usb_handle;
static CircularBuffer g_mock_usb_rx_buffer;
static uint8_t g_mock_usb_rx_data[SCPI_TEST_USB_BUFFER_SIZE];
static uint32_t g_mock_system_tick;

void setUp(void)
{
    // Initialize test fixtures
    g_mock_usb_handle = (USB_Handle *)0x12345678; // Mock handle
    g_scpi_test_captured_response_len = 0;
    g_mock_system_tick = 1000; // Start at 1 second
    g_scpi_test_injected_data_len = 0;

    // Clear buffers
    memset(g_scpi_test_captured_response, 0, sizeof(g_scpi_test_captured_response));
    memset(g_scpi_test_injected_data, 0, sizeof(g_scpi_test_injected_data));

    // Initialize circular buffer for USB RX simulation
    circular_buffer_init(&g_mock_usb_rx_buffer, g_mock_usb_rx_data, SCPI_TEST_USB_BUFFER_SIZE);

    // Initialize mocks
    mock_usb_Init();
    mock_system_Init();
}

void tearDown(void)
{
    // Clean up protocol if initialized
    if (protocol_is_initialized()) {
        // Set up mock expectations for cleanup
        USB_deinit_Ignore();
        protocol_deinit();
    }

    // Clean up mocks
    mock_usb_Destroy();
    mock_system_Destroy();
}

// ============================================================================
// Test Helper Functions
// ============================================================================

/**
 * @brief Mock USB_write implementation that captures response data
 */
static uint32_t mock_usb_write_capture(USB_Handle *handle, uint8_t const *data, uint32_t len, int cmock_num_calls)
{
    (void)handle;
    (void)cmock_num_calls;

    if (g_scpi_test_captured_response_len + len < SCPI_TEST_RESPONSE_BUFFER_SIZE) {
        memcpy(g_scpi_test_captured_response + g_scpi_test_captured_response_len, data, len);
        g_scpi_test_captured_response_len += len;
        g_scpi_test_captured_response[g_scpi_test_captured_response_len] = '\0';
    }

    return len;
}

/**
 * @brief Mock USB_read implementation that returns injected data
 */
static uint32_t mock_usb_read_inject(USB_Handle *handle, uint8_t *buffer, uint32_t max_len, int cmock_num_calls)
{
    (void)handle;
    (void)cmock_num_calls;

    if (g_scpi_test_injected_data_len == 0) {
        return 0;
    }

    uint32_t to_copy = (g_scpi_test_injected_data_len > max_len) ? max_len : (uint32_t)g_scpi_test_injected_data_len;
    memcpy(buffer, g_scpi_test_injected_data, to_copy);

    // Shift remaining data
    memmove(g_scpi_test_injected_data, g_scpi_test_injected_data + to_copy, g_scpi_test_injected_data_len - to_copy);
    g_scpi_test_injected_data_len -= to_copy;

    return to_copy;
}

/**
 * @brief Mock USB_rx_ready that returns whether injected data is available
 */
static bool mock_usb_rx_ready_check(USB_Handle *handle, int cmock_num_calls)
{
    (void)handle;
    (void)cmock_num_calls;
    return g_scpi_test_injected_data_len > 0;
}

/**
 * @brief Mock SYSTEM_get_tick implementation
 */
static uint32_t mock_system_get_tick_impl(int cmock_num_calls)
{
    (void)cmock_num_calls;
    return g_mock_system_tick;
}

/**
 * @brief Mock SYSTEM_get_tick implementation that advances time on each call
 */
static uint32_t mock_system_get_tick_advancing(int cmock_num_calls)
{
    // Advance time by 200ms on each call to simulate time passing
    // After 6 calls (1200ms), we'll exceed the 1000ms timeout
    g_mock_system_tick += 200;
    return g_mock_system_tick;
}

/**
 * @brief Helper to advance mock system time
 */
static void advance_system_time(uint32_t ms)
{
    g_mock_system_tick += ms;
}

// ============================================================================
// Protocol API Lifecycle Tests
// ============================================================================

void test_protocol_init_success(void)
{
    // Arrange
    USB_init_ExpectAndReturn(0, NULL, g_mock_usb_handle);
    USB_init_IgnoreArg_rx_buffer();
    USB_set_rx_callback_Expect(g_mock_usb_handle, NULL, 1);
    USB_set_rx_callback_IgnoreArg_callback(); // Don't care about callback function

    // Act
    bool result = protocol_init();

    // Assert
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(protocol_is_initialized());
}

void test_protocol_init_usb_failure(void)
{
    // Arrange
    USB_init_ExpectAndReturn(0, NULL, NULL);
    USB_init_IgnoreArg_rx_buffer(); // USB init fails

    // Act
    bool result = protocol_init();

    // Assert
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_FALSE(protocol_is_initialized());
}

void test_protocol_init_multiple_calls(void)
{
    // Arrange - first init succeeds
    USB_init_ExpectAndReturn(0, NULL, g_mock_usb_handle);
    USB_init_IgnoreArg_rx_buffer();
    USB_set_rx_callback_Expect(g_mock_usb_handle, NULL, 1);
    USB_set_rx_callback_IgnoreArg_callback();

    bool first_result = protocol_init();
    TEST_ASSERT_TRUE(first_result);
    TEST_ASSERT_TRUE(protocol_is_initialized());

    // Act - second init should succeed without calling USB_init again
    bool second_result = protocol_init();

    // Assert
    TEST_ASSERT_TRUE(second_result);
    TEST_ASSERT_TRUE(protocol_is_initialized());
}

void test_protocol_deinit_when_initialized(void)
{
    // Arrange - initialize first
    USB_init_ExpectAndReturn(0, NULL, g_mock_usb_handle); USB_init_IgnoreArg_rx_buffer();
    USB_set_rx_callback_Expect(g_mock_usb_handle, NULL, 1);
    USB_set_rx_callback_IgnoreArg_callback();

    bool init_result = protocol_init();
    TEST_ASSERT_TRUE(init_result);

    // Set expectations for deinit
    USB_deinit_Expect(g_mock_usb_handle);

    // Act
    protocol_deinit();

    // Assert
    TEST_ASSERT_FALSE(protocol_is_initialized());
}

void test_protocol_deinit_when_not_initialized(void)
{
    // Act
    protocol_deinit();

    // Assert - No crash, no expectations needed
    TEST_ASSERT_FALSE(protocol_is_initialized());
}

void test_protocol_task_when_not_initialized(void)
{
    // Act
    protocol_task();

    // Assert - Should not crash, no USB operations expected
    TEST_PASS();
}

void test_protocol_task_with_no_usb_data(void)
{
    // Arrange
    USB_init_ExpectAndReturn(0, NULL, g_mock_usb_handle);
    USB_init_IgnoreArg_rx_buffer();
    USB_set_rx_callback_Expect(g_mock_usb_handle, NULL, 1);
    USB_set_rx_callback_IgnoreArg_callback();

    protocol_init();

    // Set expectations for task with no data
    USB_task_Expect(g_mock_usb_handle);
    USB_rx_ready_ExpectAndReturn(g_mock_usb_handle, false);

    // Act
    protocol_task();

    // Assert - No crash, expectations verified by mock
    TEST_PASS();
}

// ============================================================================
// SCPI Command Tests - IEEE 488.2 Standard Commands
// ============================================================================

void test_scpi_idn_query(void)
{
    // Arrange
    USB_init_ExpectAndReturn(0, NULL, g_mock_usb_handle); USB_init_IgnoreArg_rx_buffer();
    USB_set_rx_callback_Ignore();
    protocol_init();

    // Set up USB communication mocks
    USB_task_Expect(g_mock_usb_handle);
    USB_rx_ready_StubWithCallback(mock_usb_rx_ready_check);
    USB_read_StubWithCallback(mock_usb_read_inject);
    USB_write_StubWithCallback(mock_usb_write_capture);

    // Inject *IDN? command
    scpi_inject_usb_command("*IDN?\n");

    // Act
    protocol_task();

    // Assert
    const char *response = scpi_get_captured_response();
    TEST_ASSERT_NOT_NULL(response);
    TEST_ASSERT_TRUE(strlen(response) > 0);
    // Should contain manufacturer, model, serial, version
    TEST_ASSERT_TRUE(strstr(response, "FOSSASIA") != NULL);
    TEST_ASSERT_TRUE(strstr(response, "PSLab") != NULL);
}

void test_scpi_rst_command(void)
{
    // Arrange
    USB_init_ExpectAndReturn(0, NULL, g_mock_usb_handle); USB_init_IgnoreArg_rx_buffer();
    USB_set_rx_callback_Ignore();
    protocol_init();

    // Set up USB communication mocks
    USB_task_Expect(g_mock_usb_handle);
    USB_rx_ready_StubWithCallback(mock_usb_rx_ready_check);
    USB_read_StubWithCallback(mock_usb_read_inject);
    USB_write_StubWithCallback(mock_usb_write_capture);

    // Inject *RST command
    scpi_inject_usb_command("*RST\n");

    // Act
    protocol_task();

    // Assert - *RST should not generate a response, but should reset state
    const char *response = scpi_get_captured_response();
    TEST_ASSERT_EQUAL(0, strlen(response)); // No response expected
}

void test_scpi_tst_query(void)
{
    // Arrange
    USB_init_ExpectAndReturn(0, NULL, g_mock_usb_handle); USB_init_IgnoreArg_rx_buffer();
    USB_set_rx_callback_Ignore();
    protocol_init();

    USB_task_Expect(g_mock_usb_handle);
    USB_rx_ready_StubWithCallback(mock_usb_rx_ready_check);
    USB_read_StubWithCallback(mock_usb_read_inject);
    USB_write_StubWithCallback(mock_usb_write_capture);

    scpi_inject_usb_command("*TST?\n");

    // Act
    protocol_task();

    // Assert
    const char *response = scpi_get_captured_response();
    TEST_ASSERT_NOT_NULL(response);
    // *TST? should return 0 for pass, non-zero for failure
    TEST_ASSERT_TRUE(strstr(response, "0") != NULL);
}

void test_scpi_system_error_query(void)
{
    // Arrange
    USB_init_ExpectAndReturn(0, NULL, g_mock_usb_handle); USB_init_IgnoreArg_rx_buffer();
    USB_set_rx_callback_Ignore();
    protocol_init();

    USB_task_Expect(g_mock_usb_handle);
    USB_rx_ready_StubWithCallback(mock_usb_rx_ready_check);
    USB_read_StubWithCallback(mock_usb_read_inject);
    USB_write_StubWithCallback(mock_usb_write_capture);

    scpi_inject_usb_command("SYST:ERR?\n");

    // Act
    protocol_task();

    // Assert
    const char *response = scpi_get_captured_response();
    TEST_ASSERT_NOT_NULL(response);
    // Should return "0,\"No error\"" when no errors
    TEST_ASSERT_TRUE(strstr(response, "0,") != NULL);
}

// ============================================================================
// USB Communication and Error Handling Tests
// ============================================================================

void test_usb_data_fragmentation(void)
{
    // Arrange
    USB_init_ExpectAndReturn(0, NULL, g_mock_usb_handle); USB_init_IgnoreArg_rx_buffer();
    USB_set_rx_callback_Ignore();
    protocol_init();

    USB_task_Expect(g_mock_usb_handle);
    USB_rx_ready_StubWithCallback(mock_usb_rx_ready_check);
    USB_read_StubWithCallback(mock_usb_read_inject);
    USB_write_StubWithCallback(mock_usb_write_capture);

    // Test fragmented command reception
    scpi_inject_usb_command("*IDN");

    protocol_task();
    const char *response1 = scpi_get_captured_response();
    TEST_ASSERT_EQUAL(0, strlen(response1)); // No response yet

    // Send remaining part
    scpi_clear_captured_response();
    scpi_inject_usb_command("?\n");

    USB_task_Expect(g_mock_usb_handle);
    USB_rx_ready_StubWithCallback(mock_usb_rx_ready_check);
    USB_read_StubWithCallback(mock_usb_read_inject);
    USB_write_StubWithCallback(mock_usb_write_capture);

    // Act
    protocol_task();

    // Assert - Should now have complete response
    const char *response2 = scpi_get_captured_response();
    TEST_ASSERT_TRUE(strlen(response2) > 0);
    TEST_ASSERT_TRUE(strstr(response2, "FOSSASIA") != NULL);
}

void test_invalid_scpi_command(void)
{
    // Arrange
    USB_init_ExpectAndReturn(0, NULL, g_mock_usb_handle); USB_init_IgnoreArg_rx_buffer();
    USB_set_rx_callback_Ignore();
    protocol_init();

    USB_task_Expect(g_mock_usb_handle);
    USB_rx_ready_StubWithCallback(mock_usb_rx_ready_check);
    USB_read_StubWithCallback(mock_usb_read_inject);
    USB_write_StubWithCallback(mock_usb_write_capture);

    scpi_inject_usb_command("INVALID:COMMAND\n");

    // Act
    protocol_task();

    // Assert - Should generate SCPI error, check with SYST:ERR?
    scpi_clear_captured_response();
    scpi_inject_usb_command("SYST:ERR?\n");

    USB_task_Expect(g_mock_usb_handle);
    USB_rx_ready_StubWithCallback(mock_usb_rx_ready_check);
    USB_read_StubWithCallback(mock_usb_read_inject);
    USB_write_StubWithCallback(mock_usb_write_capture);

    protocol_task();

    const char *error_response = scpi_get_captured_response();
    TEST_ASSERT_TRUE(strlen(error_response) > 0);
    TEST_ASSERT_FALSE(strstr(error_response, "0,") != NULL); // Should not be "No error"
}

void test_multiple_commands_in_sequence(void)
{
    // Arrange
    USB_init_ExpectAndReturn(0, NULL, g_mock_usb_handle); USB_init_IgnoreArg_rx_buffer();
    USB_set_rx_callback_Ignore();
    protocol_init();

    // Process multiple commands in sequence
    for (int i = 0; i < 3; i++) {
        USB_task_Expect(g_mock_usb_handle);
        USB_rx_ready_StubWithCallback(mock_usb_rx_ready_check);
        USB_read_StubWithCallback(mock_usb_read_inject);
        USB_write_StubWithCallback(mock_usb_write_capture);
    }

    scpi_inject_usb_command("*IDN?\n*TST?\n*IDN?\n");

    // Act - Process all commands
    for (int i = 0; i < 3; i++) {
        protocol_task();
    }

    // Assert - Should have responses from all commands
    const char *response = scpi_get_captured_response();
    TEST_ASSERT_TRUE(strlen(response) > 0);
}

// ============================================================================
// State Management and Reset Tests
// ============================================================================

void test_protocol_state_reset(void)
{
    // Arrange - Initialize and set up some state
    USB_init_ExpectAndReturn(0, NULL, g_mock_usb_handle); USB_init_IgnoreArg_rx_buffer();
    USB_set_rx_callback_Ignore();
    protocol_init();

    // Reset and expect proper cleanup
    scpi_clear_captured_response();
    USB_task_Expect(g_mock_usb_handle);
    USB_rx_ready_StubWithCallback(mock_usb_rx_ready_check);
    USB_read_StubWithCallback(mock_usb_read_inject);
    USB_write_StubWithCallback(mock_usb_write_capture);

    scpi_inject_usb_command("*RST\n");

    // Act
    protocol_task();

    // Assert - State should be reset, no response from *RST
    const char *response = scpi_get_captured_response();
    TEST_ASSERT_EQUAL(0, strlen(response));

    // Verify state is properly reset by checking error queue
    scpi_clear_captured_response();
    USB_task_Expect(g_mock_usb_handle);
    USB_rx_ready_StubWithCallback(mock_usb_rx_ready_check);
    USB_read_StubWithCallback(mock_usb_read_inject);
    USB_write_StubWithCallback(mock_usb_write_capture);

    scpi_inject_usb_command("SYST:ERR?\n");
    protocol_task();

    const char *error_response = scpi_get_captured_response();
    TEST_ASSERT_TRUE(strstr(error_response, "0,") != NULL); // Should be "No error"
}