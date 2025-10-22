/**
 * @file test_protocol_dmm.c
 * @brief Unit tests for DMM-specific SCPI protocol functionality
 *
 * This file contains unit tests for the DMM protocol functionality covering:
 * - DMM SCPI command processing (CONF:VOLT:DC, INIT:VOLT:DC, FETC:VOLT:DC, etc.)
 * - DMM error handling and recovery
 * - DMM state management and caching
 * - DMM timeout handling
 *
 * The DMM protocol implements SCPI commands for voltage measurement functionality.
 *
 * @author PSLab Team
 * @date 2025-10-02
 */

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "unity.h"
#include "mock_usb.h"
#include "mock_dmm.h"
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
static DMM_Handle *g_mock_dmm_handle;
static CircularBuffer g_mock_usb_rx_buffer;
static uint8_t g_mock_usb_rx_data[SCPI_TEST_USB_BUFFER_SIZE];
static uint32_t g_mock_system_tick;

void setUp(void)
{
    // Initialize test fixtures
    g_mock_usb_handle = (USB_Handle *)0x12345678; // Mock handle
    g_mock_dmm_handle = (DMM_Handle *)0x87654321; // Mock handle
    g_scpi_test_captured_response_len = 0;
    g_mock_system_tick = 1000; // Start at 1 second
    g_scpi_test_injected_data_len = 0;

    // Clear buffers
    scpi_clear_captured_response();
    memset(g_scpi_test_injected_data, 0, sizeof(g_scpi_test_injected_data));

    // Initialize circular buffer for USB RX simulation
    circular_buffer_init(&g_mock_usb_rx_buffer, g_mock_usb_rx_data, SCPI_TEST_USB_BUFFER_SIZE);

    // Initialize mocks
    mock_usb_Init();
    mock_dmm_Init();
    mock_system_Init();
}

void tearDown(void)
{
    // Clean up protocol if initialized
    if (protocol_is_initialized()) {
        // Set up mock expectations for cleanup
        USB_deinit_Ignore();
        DMM_deinit_Ignore();
        protocol_deinit();
    }

    // Clean up mocks
    mock_usb_Destroy();
    mock_dmm_Destroy();
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

/**
 * @brief Helper to initialize protocol for DMM tests
 */
static void setup_protocol_for_dmm_test(void)
{
    USB_init_ExpectAndReturn(0, NULL, g_mock_usb_handle);
    USB_init_IgnoreArg_rx_buffer();
    USB_set_rx_callback_Ignore();
    protocol_init();

    USB_task_Expect(g_mock_usb_handle);
    USB_rx_ready_StubWithCallback(scpi_mock_usb_rx_ready_check);
    USB_read_StubWithCallback(scpi_mock_usb_read_inject);
    USB_write_StubWithCallback(scpi_mock_usb_write_capture);
}

// ============================================================================
// SCPI Command Tests - Instrument-Specific Voltage Commands
// ============================================================================

void test_scpi_configure_voltage_dc_success(void)
{
    // Arrange
    setup_protocol_for_dmm_test();

    // Mock DMM initialization for configuration validation
    DMM_init_ExpectAndReturn(NULL, g_mock_dmm_handle);
    DMM_init_IgnoreArg_config();
    DMM_deinit_Expect(NULL); DMM_deinit_IgnoreArg_handle();

    scpi_inject_usb_command("CONF:VOLT:DC\n");

    // Act
    protocol_task();

    // Assert
    const char *response = scpi_get_captured_response();
    TEST_ASSERT_EQUAL(0, strlen(response)); // CONF command generates no response
}

void test_scpi_configure_voltage_dc_with_channel(void)
{
    // Arrange
    setup_protocol_for_dmm_test();

    // Mock DMM initialization for configuration validation
    DMM_init_ExpectAndReturn(NULL, g_mock_dmm_handle);
    DMM_init_IgnoreArg_config();
    DMM_deinit_Expect(NULL); DMM_deinit_IgnoreArg_handle();

    scpi_inject_usb_command("CONF:VOLT:DC 5\n"); // Channel 5

    // Act
    protocol_task();

    // Assert
    const char *response = scpi_get_captured_response();
    TEST_ASSERT_EQUAL(0, strlen(response)); // CONF command generates no response
}

void test_scpi_configure_voltage_dc_invalid_channel(void)
{
    // Arrange
    setup_protocol_for_dmm_test();

    // Mock DMM initialization failure for invalid configuration
    DMM_init_ExpectAndThrow(NULL, ERROR_INVALID_ARGUMENT);
    DMM_init_IgnoreArg_config();

    scpi_inject_usb_command("CONF:VOLT:DC 999\n"); // Invalid channel

    // Act
    protocol_task();

    // Assert - Should generate SCPI error, check with SYST:ERR?
    scpi_clear_captured_response();
    scpi_inject_usb_command("SYST:ERR?\n");

    scpi_run_protocol_with_usb_mocks(g_mock_usb_handle);

    const char *error_response = scpi_get_captured_response();
    TEST_ASSERT_SCPI_ERROR(error_response);
}

void test_scpi_initiate_voltage_dc_success(void)
{
    // Arrange
    setup_protocol_for_dmm_test();

    // Mock successful DMM initialization
    DMM_deinit_Expect(NULL); DMM_deinit_IgnoreArg_handle(); // Cleanup any existing handle
    DMM_init_ExpectAndReturn(NULL, g_mock_dmm_handle); DMM_init_IgnoreArg_config();

    scpi_inject_usb_command("INIT:VOLT:DC\n");

    // Act
    protocol_task();

    // Assert
    const char *response = scpi_get_captured_response();
    TEST_ASSERT_EQUAL(0, strlen(response)); // INIT command generates no response
}

void test_scpi_initiate_voltage_dc_dmm_failure(void)
{
    // Arrange
    setup_protocol_for_dmm_test();

    // Mock DMM initialization failure
    DMM_deinit_Expect(NULL);
    DMM_deinit_IgnoreArg_handle();
    DMM_init_ExpectAndThrow(NULL, ERROR_HARDWARE_FAULT);
    DMM_init_IgnoreArg_config();

    scpi_inject_usb_command("INIT:VOLT:DC\n");

    // Act
    protocol_task();

    // Assert - Should generate SCPI error
    scpi_clear_captured_response();
    scpi_inject_usb_command("SYST:ERR?\n");

    USB_task_Expect(g_mock_usb_handle);
    USB_rx_ready_StubWithCallback(mock_usb_rx_ready_check);
    USB_read_StubWithCallback(mock_usb_read_inject);
    USB_write_StubWithCallback(mock_usb_write_capture);

    protocol_task();

    const char *error_response = scpi_get_captured_response();
    TEST_ASSERT_SCPI_ERROR(error_response);
}

void test_scpi_fetch_voltage_dc_with_cached_value(void)
{
    // Arrange - Initialize and set up a cached voltage reading
    setup_protocol_for_dmm_test();

    // First, configure and initiate to get a cached reading
    DMM_init_ExpectAndReturn(NULL, g_mock_dmm_handle); DMM_init_IgnoreArg_config();
    DMM_deinit_Expect(NULL); DMM_deinit_IgnoreArg_handle();

    scpi_inject_usb_command("CONF:VOLT:DC\n");
    protocol_task();

    scpi_clear_captured_response();

    USB_task_Expect(g_mock_usb_handle);
    USB_rx_ready_StubWithCallback(mock_usb_rx_ready_check);
    USB_read_StubWithCallback(mock_usb_read_inject);
    USB_write_StubWithCallback(mock_usb_write_capture);
    DMM_deinit_Expect(NULL); DMM_deinit_IgnoreArg_handle();
    DMM_init_ExpectAndReturn(NULL, g_mock_dmm_handle); DMM_init_IgnoreArg_config();

    scpi_inject_usb_command("INIT:VOLT:DC\n");
    protocol_task();

    scpi_clear_captured_response();

    // Now test FETCH with cached value
    USB_task_Expect(g_mock_usb_handle);
    USB_rx_ready_StubWithCallback(mock_usb_rx_ready_check);
    USB_read_StubWithCallback(mock_usb_read_inject);
    USB_write_StubWithCallback(mock_usb_write_capture);

    // Mock reading from active DMM handle
    SYSTEM_get_tick_StubWithCallback(mock_system_get_tick_impl);
    DMM_read_voltage_ExpectAndReturn(NULL, NULL, true); DMM_read_voltage_IgnoreArg_handle(); DMM_read_voltage_IgnoreArg_voltage_out();
    FIXED_Q1616 voltage_out = FIXED_FROM_FLOAT(1.65f);
    DMM_read_voltage_ReturnThruPtr_voltage_out(&voltage_out);
    DMM_deinit_Expect(NULL); DMM_deinit_IgnoreArg_handle();

    scpi_inject_usb_command("FETC:VOLT:DC?\n");

    // Act
    protocol_task();

    // Assert
    const char *response = scpi_get_captured_response();
    TEST_ASSERT_TRUE(strlen(response) > 0);
    // Should return voltage in millivolts
    // Fixed-point representation results in 1649
    TEST_ASSERT_TRUE(strstr(response, "1649") != NULL);
}

void test_scpi_fetch_voltage_dc_no_init(void)
{
    // Arrange - Try to fetch without INIT
    setup_protocol_for_dmm_test();

    scpi_inject_usb_command("FETC:VOLT:DC?\n");

    // Act
    protocol_task();

    // Assert - Should generate SCPI error
    scpi_clear_captured_response();
    scpi_inject_usb_command("SYST:ERR?\n");

    USB_task_Expect(g_mock_usb_handle);
    USB_rx_ready_StubWithCallback(mock_usb_rx_ready_check);
    USB_read_StubWithCallback(mock_usb_read_inject);
    USB_write_StubWithCallback(mock_usb_write_capture);

    protocol_task();

    const char *error_response = scpi_get_captured_response();
    TEST_ASSERT_SCPI_ERROR(error_response);
}

void test_scpi_read_voltage_dc_complete_flow(void)
{
    // Arrange
    setup_protocol_for_dmm_test();

    // Expect INIT sequence
    DMM_deinit_Expect(NULL); DMM_deinit_IgnoreArg_handle();
    DMM_init_ExpectAndReturn(NULL, g_mock_dmm_handle); DMM_init_IgnoreArg_config();

    // Expect FETCH sequence
    SYSTEM_get_tick_StubWithCallback(mock_system_get_tick_impl);
    DMM_read_voltage_ExpectAndReturn(NULL, NULL, true); DMM_read_voltage_IgnoreArg_handle(); DMM_read_voltage_IgnoreArg_voltage_out();
    FIXED_Q1616 voltage_out = FIXED_FROM_FLOAT(2.75f);
    DMM_read_voltage_ReturnThruPtr_voltage_out(&voltage_out);
    DMM_deinit_Expect(NULL); DMM_deinit_IgnoreArg_handle();

    scpi_inject_usb_command("READ:VOLT:DC?\n");

    // Act
    protocol_task();

    // Assert
    const char *response = scpi_get_captured_response();
    TEST_ASSERT_TRUE(strlen(response) > 0);
    TEST_ASSERT_TRUE(strstr(response, "2750") != NULL); // 2.75V = 2750mV
}

void test_scpi_measure_voltage_dc_complete_flow(void)
{
    // Arrange
    setup_protocol_for_dmm_test();

    // Expect CONF sequence (validation)
    DMM_init_ExpectAndReturn(NULL, g_mock_dmm_handle); DMM_init_IgnoreArg_config();
    DMM_deinit_Expect(NULL); DMM_deinit_IgnoreArg_handle();

    // Expect READ sequence (INIT + FETCH)
    DMM_deinit_Expect(NULL); DMM_deinit_IgnoreArg_handle();
    DMM_init_ExpectAndReturn(NULL, g_mock_dmm_handle); DMM_init_IgnoreArg_config();
    SYSTEM_get_tick_StubWithCallback(mock_system_get_tick_impl);
    DMM_read_voltage_ExpectAndReturn(NULL, NULL, true); DMM_read_voltage_IgnoreArg_handle(); DMM_read_voltage_IgnoreArg_voltage_out();
    FIXED_Q1616 voltage_out = FIXED_FROM_FLOAT(0.85f);
    DMM_read_voltage_ReturnThruPtr_voltage_out(&voltage_out);
    DMM_deinit_Expect(NULL); DMM_deinit_IgnoreArg_handle();

    scpi_inject_usb_command("MEAS:VOLT:DC?\n");

    // Act
    protocol_task();

    // Assert
    const char *response = scpi_get_captured_response();
    TEST_ASSERT_TRUE(strlen(response) > 0);
    // Fixed-point representation results in 849
    TEST_ASSERT_TRUE(strstr(response, "849") != NULL);
}

void test_dmm_read_timeout_handling(void)
{
    // Arrange
    setup_protocol_for_dmm_test();

    // Mock DMM init and read timeout scenario
    DMM_deinit_Expect(NULL); DMM_deinit_IgnoreArg_handle();
    DMM_init_ExpectAndReturn(NULL, g_mock_dmm_handle); DMM_init_IgnoreArg_config();

    // Use the advancing time mock to simulate timeout
    // Time will advance by 200ms on each call, causing timeout after ~6 calls
    SYSTEM_get_tick_StubWithCallback(mock_system_get_tick_advancing);

    // DMM_read_voltage should return false (not ready) for all calls during timeout
    DMM_read_voltage_IgnoreAndReturn(false);

    scpi_inject_usb_command("READ:VOLT:DC?\n");

    // Act
    protocol_task();

    // Assert - Should generate SCPI timeout error
    scpi_clear_captured_response();
    scpi_inject_usb_command("SYST:ERR?\n");

    USB_task_Expect(g_mock_usb_handle);
    USB_rx_ready_StubWithCallback(mock_usb_rx_ready_check);
    USB_read_StubWithCallback(mock_usb_read_inject);
    USB_write_StubWithCallback(mock_usb_write_capture);

    protocol_task();

    const char *error_response = scpi_get_captured_response();
    TEST_ASSERT_SCPI_ERROR(error_response);
}

void test_dmm_configure_with_different_channels(void)
{
    // Test configuration with different ADC channels
    setup_protocol_for_dmm_test();

    // Test channel 0
    DMM_init_ExpectAndReturn(NULL, g_mock_dmm_handle);
    DMM_init_IgnoreArg_config();
    DMM_deinit_Expect(NULL); DMM_deinit_IgnoreArg_handle();

    scpi_inject_usb_command("CONF:VOLT:DC 0\n");
    protocol_task();

    scpi_clear_captured_response();

    // Test channel 7 (max valid channel)
    USB_task_Expect(g_mock_usb_handle);
    USB_rx_ready_StubWithCallback(mock_usb_rx_ready_check);
    USB_read_StubWithCallback(mock_usb_read_inject);
    USB_write_StubWithCallback(mock_usb_write_capture);

    DMM_init_ExpectAndReturn(NULL, g_mock_dmm_handle);
    DMM_init_IgnoreArg_config();
    DMM_deinit_Expect(NULL); DMM_deinit_IgnoreArg_handle();

    scpi_inject_usb_command("CONF:VOLT:DC 7\n");

    // Act
    protocol_task();

    // Assert
    const char *response = scpi_get_captured_response();
    TEST_ASSERT_EQUAL(0, strlen(response)); // CONF command generates no response
}

void test_dmm_multiple_measurements_same_channel(void)
{
    // Test multiple measurements on the same configured channel
    setup_protocol_for_dmm_test();

    // Configure once
    DMM_init_ExpectAndReturn(NULL, g_mock_dmm_handle);
    DMM_init_IgnoreArg_config();
    DMM_deinit_Expect(NULL); DMM_deinit_IgnoreArg_handle();

    scpi_inject_usb_command("CONF:VOLT:DC 2\n");
    protocol_task();

    scpi_clear_captured_response();

    // First measurement
    USB_task_Expect(g_mock_usb_handle);
    USB_rx_ready_StubWithCallback(mock_usb_rx_ready_check);
    USB_read_StubWithCallback(mock_usb_read_inject);
    USB_write_StubWithCallback(mock_usb_write_capture);

    DMM_deinit_Expect(NULL); DMM_deinit_IgnoreArg_handle();
    DMM_init_ExpectAndReturn(NULL, g_mock_dmm_handle); DMM_init_IgnoreArg_config();
    SYSTEM_get_tick_StubWithCallback(mock_system_get_tick_impl);
    DMM_read_voltage_ExpectAndReturn(NULL, NULL, true);
    DMM_read_voltage_IgnoreArg_handle(); DMM_read_voltage_IgnoreArg_voltage_out();
    FIXED_Q1616 voltage_out1 = FIXED_FROM_FLOAT(1.23f);
    DMM_read_voltage_ReturnThruPtr_voltage_out(&voltage_out1);
    DMM_deinit_Expect(NULL); DMM_deinit_IgnoreArg_handle();

    scpi_inject_usb_command("READ:VOLT:DC?\n");
    protocol_task();

    TEST_ASSERT_TRUE(strstr(scpi_get_captured_response(), "1229") != NULL);

    scpi_clear_captured_response();

    // Second measurement
    USB_task_Expect(g_mock_usb_handle);
    USB_rx_ready_StubWithCallback(mock_usb_rx_ready_check);
    USB_read_StubWithCallback(mock_usb_read_inject);
    USB_write_StubWithCallback(mock_usb_write_capture);

    DMM_deinit_Expect(NULL); DMM_deinit_IgnoreArg_handle();
    DMM_init_ExpectAndReturn(NULL, g_mock_dmm_handle); DMM_init_IgnoreArg_config();
    SYSTEM_get_tick_StubWithCallback(mock_system_get_tick_impl);
    DMM_read_voltage_ExpectAndReturn(NULL, NULL, true);
    DMM_read_voltage_IgnoreArg_handle(); DMM_read_voltage_IgnoreArg_voltage_out();
    FIXED_Q1616 voltage_out2 = FIXED_FROM_FLOAT(3.45f);
    DMM_read_voltage_ReturnThruPtr_voltage_out(&voltage_out2);
    DMM_deinit_Expect(NULL); DMM_deinit_IgnoreArg_handle();

    scpi_inject_usb_command("READ:VOLT:DC?\n");

    // Act
    protocol_task();

    // Assert
    TEST_ASSERT_TRUE(strstr(scpi_get_captured_response(), "3449") != NULL);
}