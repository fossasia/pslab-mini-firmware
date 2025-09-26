/**
 * @file test_protocol.c
 * @brief Comprehensive unit tests for the SCPI protocol module
 *
 * This file contains unit tests for the protocol API covering:
 * - Protocol lifecycle management (init/deinit/task)
 * - SCPI command processing (IEEE 488.2 and instrument-specific)
 * - USB communication handling
 * - Error handling and recovery
 * - State management and caching
 *
 * The protocol module uses USB CDC as transport and implements SCPI commands
 * for instrument control, primarily voltage measurement functionality.
 *
 * @author PSLab Team
 * @date 2025-09-25
 */

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "unity.h"
#include "mock_usb.h"
#include "mock_dmm.h"
#include "mock_system.h"

#include "util/error.h"
#include "util/fixed_point.h"
#include "util/util.h"

#include "application/protocol.h"

// Test buffer sizes matching protocol implementation
#define TEST_USB_BUFFER_SIZE 512
#define TEST_RESPONSE_BUFFER_SIZE 1024

// Test fixtures and mock data
static USB_Handle *g_mock_usb_handle;
static DMM_Handle *g_mock_dmm_handle;
static CircularBuffer g_mock_usb_rx_buffer;
static uint8_t g_mock_usb_rx_data[TEST_USB_BUFFER_SIZE];
static char g_captured_response[TEST_RESPONSE_BUFFER_SIZE];
static size_t g_captured_response_len;
static bool g_usb_rx_ready;
static uint32_t g_mock_system_tick;

// Mock USB data injection queue
static char g_injected_data[TEST_USB_BUFFER_SIZE];
static size_t g_injected_data_len;

void setUp(void)
{
    // Initialize test fixtures
    g_mock_usb_handle = (USB_Handle *)0x12345678; // Mock handle
    g_mock_dmm_handle = (DMM_Handle *)0x87654321; // Mock handle
    g_captured_response_len = 0;
    g_usb_rx_ready = false;
    g_mock_system_tick = 1000; // Start at 1 second
    g_injected_data_len = 0;

    // Clear buffers
    memset(g_captured_response, 0, sizeof(g_captured_response));
    memset(g_injected_data, 0, sizeof(g_injected_data));

    // Initialize circular buffer for USB RX simulation
    circular_buffer_init(&g_mock_usb_rx_buffer, g_mock_usb_rx_data, TEST_USB_BUFFER_SIZE);

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

    if (g_captured_response_len + len < TEST_RESPONSE_BUFFER_SIZE) {
        memcpy(g_captured_response + g_captured_response_len, data, len);
        g_captured_response_len += len;
        g_captured_response[g_captured_response_len] = '\0';
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

    if (g_injected_data_len == 0) {
        return 0;
    }

    uint32_t to_copy = (g_injected_data_len > max_len) ? max_len : (uint32_t)g_injected_data_len;
    memcpy(buffer, g_injected_data, to_copy);

    // Shift remaining data
    memmove(g_injected_data, g_injected_data + to_copy, g_injected_data_len - to_copy);
    g_injected_data_len -= to_copy;

    return to_copy;
}

/**
 * @brief Mock USB_rx_ready that returns whether injected data is available
 */
static bool mock_usb_rx_ready_check(USB_Handle *handle, int cmock_num_calls)
{
    (void)handle;
    (void)cmock_num_calls;
    return g_injected_data_len > 0;
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
 * @brief Helper to inject USB command data for testing
 */
static void inject_usb_command(const char *command)
{
    size_t cmd_len = strlen(command);
    if (g_injected_data_len + cmd_len < TEST_USB_BUFFER_SIZE) {
        memcpy(g_injected_data + g_injected_data_len, command, cmd_len);
        g_injected_data_len += cmd_len;
    }
}

/**
 * @brief Helper to get captured USB response
 */
static const char *get_captured_response(void)
{
    return g_captured_response;
}

/**
 * @brief Helper to clear captured response
 */
static void clear_captured_response(void)
{
    g_captured_response_len = 0;
    memset(g_captured_response, 0, sizeof(g_captured_response));
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
    DMM_deinit_Expect(NULL); DMM_deinit_IgnoreArg_handle(); // Should be called even if no DMM handle
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
    inject_usb_command("*IDN?\n");

    // Act
    protocol_task();

    // Assert
    const char *response = get_captured_response();
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

    // Expect DMM to be deinitialized during reset
    DMM_deinit_Expect(NULL); DMM_deinit_IgnoreArg_handle();

    // Inject *RST command
    inject_usb_command("*RST\n");

    // Act
    protocol_task();

    // Assert - *RST should not generate a response, but should reset state
    const char *response = get_captured_response();
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

    inject_usb_command("*TST?\n");

    // Act
    protocol_task();

    // Assert
    const char *response = get_captured_response();
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

    inject_usb_command("SYST:ERR?\n");

    // Act
    protocol_task();

    // Assert
    const char *response = get_captured_response();
    TEST_ASSERT_NOT_NULL(response);
    // Should return "0,\"No error\"" when no errors
    TEST_ASSERT_TRUE(strstr(response, "0,") != NULL);
}

// ============================================================================
// SCPI Command Tests - Instrument-Specific Voltage Commands
// ============================================================================

void test_scpi_configure_voltage_dc_success(void)
{
    // Arrange
    USB_init_ExpectAndReturn(0, NULL, g_mock_usb_handle); USB_init_IgnoreArg_rx_buffer();
    USB_set_rx_callback_Ignore();
    protocol_init();

    USB_task_Expect(g_mock_usb_handle);
    USB_rx_ready_StubWithCallback(mock_usb_rx_ready_check);
    USB_read_StubWithCallback(mock_usb_read_inject);
    USB_write_StubWithCallback(mock_usb_write_capture);

    // Mock DMM initialization for configuration validation
    DMM_init_ExpectAndReturn(NULL, g_mock_dmm_handle);
    DMM_init_IgnoreArg_config();
    DMM_deinit_Expect(NULL); DMM_deinit_IgnoreArg_handle();

    inject_usb_command("CONF:VOLT:DC\n");

    // Act
    protocol_task();

    // Assert
    const char *response = get_captured_response();
    TEST_ASSERT_EQUAL(0, strlen(response)); // CONF command generates no response
}

void test_scpi_configure_voltage_dc_with_channel(void)
{
    // Arrange
    USB_init_ExpectAndReturn(0, NULL, g_mock_usb_handle); USB_init_IgnoreArg_rx_buffer();
    USB_set_rx_callback_Ignore();
    protocol_init();

    USB_task_Expect(g_mock_usb_handle);
    USB_rx_ready_StubWithCallback(mock_usb_rx_ready_check);
    USB_read_StubWithCallback(mock_usb_read_inject);
    USB_write_StubWithCallback(mock_usb_write_capture);

    // Mock DMM initialization for configuration validation
    DMM_init_ExpectAndReturn(NULL, g_mock_dmm_handle);
    DMM_init_IgnoreArg_config();
    DMM_deinit_Expect(NULL); DMM_deinit_IgnoreArg_handle();

    inject_usb_command("CONF:VOLT:DC 5\n"); // Channel 5

    // Act
    protocol_task();

    // Assert
    const char *response = get_captured_response();
    TEST_ASSERT_EQUAL(0, strlen(response)); // CONF command generates no response
}

void test_scpi_configure_voltage_dc_invalid_channel(void)
{
    // Arrange
    USB_init_ExpectAndReturn(0, NULL, g_mock_usb_handle); USB_init_IgnoreArg_rx_buffer();
    USB_set_rx_callback_Ignore();
    protocol_init();

    USB_task_Expect(g_mock_usb_handle);
    USB_rx_ready_StubWithCallback(mock_usb_rx_ready_check);
    USB_read_StubWithCallback(mock_usb_read_inject);
    USB_write_StubWithCallback(mock_usb_write_capture);

    // Mock DMM initialization failure for invalid configuration
    DMM_init_ExpectAndThrow(NULL, ERROR_INVALID_ARGUMENT);
    DMM_init_IgnoreArg_config();

    inject_usb_command("CONF:VOLT:DC 999\n"); // Invalid channel

    // Act
    protocol_task();

    // Assert - Should generate SCPI error, check with SYST:ERR?
    clear_captured_response();
    inject_usb_command("SYST:ERR?\n");

    USB_task_Expect(g_mock_usb_handle);
    USB_rx_ready_StubWithCallback(mock_usb_rx_ready_check);
    USB_read_StubWithCallback(mock_usb_read_inject);
    USB_write_StubWithCallback(mock_usb_write_capture);

    protocol_task();

    const char *error_response = get_captured_response();
    TEST_ASSERT_TRUE(strlen(error_response) > 0);
    TEST_ASSERT_FALSE(strstr(error_response, "0,") != NULL); // Should not be "No error"
}

void test_scpi_initiate_voltage_dc_success(void)
{
    // Arrange
    USB_init_ExpectAndReturn(0, NULL, g_mock_usb_handle); USB_init_IgnoreArg_rx_buffer();
    USB_set_rx_callback_Ignore();
    protocol_init();

    USB_task_Expect(g_mock_usb_handle);
    USB_rx_ready_StubWithCallback(mock_usb_rx_ready_check);
    USB_read_StubWithCallback(mock_usb_read_inject);
    USB_write_StubWithCallback(mock_usb_write_capture);

    // Mock successful DMM initialization
    DMM_deinit_Expect(NULL); DMM_deinit_IgnoreArg_handle(); // Cleanup any existing handle
    DMM_init_ExpectAndReturn(NULL, g_mock_dmm_handle); DMM_init_IgnoreArg_config();

    inject_usb_command("INIT:VOLT:DC\n");

    // Act
    protocol_task();

    // Assert
    const char *response = get_captured_response();
    TEST_ASSERT_EQUAL(0, strlen(response)); // INIT command generates no response
}

void test_scpi_initiate_voltage_dc_dmm_failure(void)
{
    // Arrange
    USB_init_ExpectAndReturn(0, NULL, g_mock_usb_handle);
    USB_init_IgnoreArg_rx_buffer();
    USB_set_rx_callback_Ignore();
    protocol_init();

    USB_task_Expect(g_mock_usb_handle);
    USB_rx_ready_StubWithCallback(mock_usb_rx_ready_check);
    USB_read_StubWithCallback(mock_usb_read_inject);
    USB_write_StubWithCallback(mock_usb_write_capture);

    // Mock DMM initialization failure
    DMM_deinit_Expect(NULL);
    DMM_deinit_IgnoreArg_handle();
    DMM_init_ExpectAndThrow(NULL, ERROR_HARDWARE_FAULT);
    DMM_init_IgnoreArg_config();

    inject_usb_command("INIT:VOLT:DC\n");

    // Act
    protocol_task();

    // Assert - Should generate SCPI error
    clear_captured_response();
    inject_usb_command("SYST:ERR?\n");

    USB_task_Expect(g_mock_usb_handle);
    USB_rx_ready_StubWithCallback(mock_usb_rx_ready_check);
    USB_read_StubWithCallback(mock_usb_read_inject);
    USB_write_StubWithCallback(mock_usb_write_capture);

    protocol_task();

    const char *error_response = get_captured_response();
    TEST_ASSERT_TRUE(strlen(error_response) > 0);
    // The response string should not start with "0,", indicating no error
    TEST_ASSERT_NOT_EQUAL(0, strncmp(error_response, "0,", 2));
}

void test_scpi_fetch_voltage_dc_with_cached_value(void)
{
    // Arrange - Initialize and set up a cached voltage reading
    USB_init_ExpectAndReturn(0, NULL, g_mock_usb_handle); USB_init_IgnoreArg_rx_buffer();
    USB_set_rx_callback_Ignore();
    protocol_init();

    // First, configure and initiate to get a cached reading
    USB_task_Expect(g_mock_usb_handle);
    USB_rx_ready_StubWithCallback(mock_usb_rx_ready_check);
    USB_read_StubWithCallback(mock_usb_read_inject);
    USB_write_StubWithCallback(mock_usb_write_capture);
    DMM_init_ExpectAndReturn(NULL, g_mock_dmm_handle); DMM_init_IgnoreArg_config();
    DMM_deinit_Expect(NULL); DMM_deinit_IgnoreArg_handle();

    inject_usb_command("CONF:VOLT:DC\n");
    protocol_task();

    clear_captured_response();

    USB_task_Expect(g_mock_usb_handle);
    USB_rx_ready_StubWithCallback(mock_usb_rx_ready_check);
    USB_read_StubWithCallback(mock_usb_read_inject);
    USB_write_StubWithCallback(mock_usb_write_capture);
    DMM_deinit_Expect(NULL); DMM_deinit_IgnoreArg_handle();
    DMM_init_ExpectAndReturn(NULL, g_mock_dmm_handle); DMM_init_IgnoreArg_config();

    inject_usb_command("INIT:VOLT:DC\n");
    protocol_task();

    clear_captured_response();

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

    inject_usb_command("FETC:VOLT:DC?\n");

    // Act
    protocol_task();

    // Assert
    const char *response = get_captured_response();
    TEST_ASSERT_TRUE(strlen(response) > 0);
    // Should return voltage in millivolts
    // Fixed-point representation results in 1649
    TEST_ASSERT_TRUE(strstr(response, "1649") != NULL);
}

void test_scpi_fetch_voltage_dc_no_init(void)
{
    // Arrange - Try to fetch without INIT
    USB_init_ExpectAndReturn(0, NULL, g_mock_usb_handle); USB_init_IgnoreArg_rx_buffer();
    USB_set_rx_callback_Ignore();
    protocol_init();

    USB_task_Expect(g_mock_usb_handle);
    USB_rx_ready_StubWithCallback(mock_usb_rx_ready_check);
    USB_read_StubWithCallback(mock_usb_read_inject);
    USB_write_StubWithCallback(mock_usb_write_capture);

    inject_usb_command("FETC:VOLT:DC?\n");

    // Act
    protocol_task();

    // Assert - Should generate SCPI error
    clear_captured_response();
    inject_usb_command("SYST:ERR?\n");

    USB_task_Expect(g_mock_usb_handle);
    USB_rx_ready_StubWithCallback(mock_usb_rx_ready_check);
    USB_read_StubWithCallback(mock_usb_read_inject);
    USB_write_StubWithCallback(mock_usb_write_capture);

    protocol_task();

    const char *error_response = get_captured_response();
    TEST_ASSERT_TRUE(strlen(error_response) > 0);
    // The response string should not start with "0,", indicating no error
    TEST_ASSERT_NOT_EQUAL(0, strncmp(error_response, "0,", 2));
}

void test_scpi_read_voltage_dc_complete_flow(void)
{
    // Arrange
    USB_init_ExpectAndReturn(0, NULL, g_mock_usb_handle); USB_init_IgnoreArg_rx_buffer();
    USB_set_rx_callback_Ignore();
    protocol_init();

    USB_task_Expect(g_mock_usb_handle);
    USB_rx_ready_StubWithCallback(mock_usb_rx_ready_check);
    USB_read_StubWithCallback(mock_usb_read_inject);
    USB_write_StubWithCallback(mock_usb_write_capture);

    // Expect INIT sequence
    DMM_deinit_Expect(NULL); DMM_deinit_IgnoreArg_handle();
    DMM_init_ExpectAndReturn(NULL, g_mock_dmm_handle); DMM_init_IgnoreArg_config();

    // Expect FETCH sequence
    SYSTEM_get_tick_StubWithCallback(mock_system_get_tick_impl);
    DMM_read_voltage_ExpectAndReturn(NULL, NULL, true); DMM_read_voltage_IgnoreArg_handle(); DMM_read_voltage_IgnoreArg_voltage_out();
    FIXED_Q1616 voltage_out = FIXED_FROM_FLOAT(2.75f);
    DMM_read_voltage_ReturnThruPtr_voltage_out(&voltage_out);
    DMM_deinit_Expect(NULL); DMM_deinit_IgnoreArg_handle();

    inject_usb_command("READ:VOLT:DC?\n");

    // Act
    protocol_task();

    // Assert
    const char *response = get_captured_response();
    TEST_ASSERT_TRUE(strlen(response) > 0);
    TEST_ASSERT_TRUE(strstr(response, "2750") != NULL); // 2.75V = 2750mV
}

void test_scpi_measure_voltage_dc_complete_flow(void)
{
    // Arrange
    USB_init_ExpectAndReturn(0, NULL, g_mock_usb_handle); USB_init_IgnoreArg_rx_buffer();
    USB_set_rx_callback_Ignore();
    protocol_init();

    USB_task_Expect(g_mock_usb_handle);
    USB_rx_ready_StubWithCallback(mock_usb_rx_ready_check);
    USB_read_StubWithCallback(mock_usb_read_inject);
    USB_write_StubWithCallback(mock_usb_write_capture);

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

    inject_usb_command("MEAS:VOLT:DC?\n");

    // Act
    protocol_task();

    // Assert
    const char *response = get_captured_response();
    TEST_ASSERT_TRUE(strlen(response) > 0);
    // Fixed-point representation results in 849
    TEST_ASSERT_TRUE(strstr(response, "849") != NULL);
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

    // Send command in fragments
    USB_task_Expect(g_mock_usb_handle);
    USB_rx_ready_StubWithCallback(mock_usb_rx_ready_check);
    USB_read_StubWithCallback(mock_usb_read_inject);
    USB_write_StubWithCallback(mock_usb_write_capture);

    inject_usb_command("*ID"); // First fragment
    protocol_task();

    // Should not generate response yet
    TEST_ASSERT_EQUAL(0, strlen(get_captured_response()));

    USB_task_Expect(g_mock_usb_handle);
    USB_rx_ready_StubWithCallback(mock_usb_rx_ready_check);
    USB_read_StubWithCallback(mock_usb_read_inject);
    USB_write_StubWithCallback(mock_usb_write_capture);

    inject_usb_command("N?\n"); // Complete the command

    // Act
    protocol_task();

    // Assert
    const char *response = get_captured_response();
    TEST_ASSERT_TRUE(strlen(response) > 0);
    TEST_ASSERT_TRUE(strstr(response, "FOSSASIA") != NULL);
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

    inject_usb_command("INVALID:COMMAND\n");

    // Act
    protocol_task();

    // Assert - Should generate SCPI error
    clear_captured_response();
    inject_usb_command("SYST:ERR?\n");

    USB_task_Expect(g_mock_usb_handle);
    USB_rx_ready_StubWithCallback(mock_usb_rx_ready_check);
    USB_read_StubWithCallback(mock_usb_read_inject);
    USB_write_StubWithCallback(mock_usb_write_capture);

    protocol_task();

    const char *error_response = get_captured_response();
    TEST_ASSERT_TRUE(strlen(error_response) > 0);
    TEST_ASSERT_FALSE(strstr(error_response, "0,") != NULL);
}

void test_dmm_read_timeout_handling(void)
{
    // Arrange
    USB_init_ExpectAndReturn(0, NULL, g_mock_usb_handle); USB_init_IgnoreArg_rx_buffer();
    USB_set_rx_callback_Ignore();
    protocol_init();

    USB_task_Expect(g_mock_usb_handle);
    USB_rx_ready_StubWithCallback(mock_usb_rx_ready_check);
    USB_read_StubWithCallback(mock_usb_read_inject);
    USB_write_StubWithCallback(mock_usb_write_capture);

    // Mock DMM init and read timeout scenario
    DMM_deinit_Expect(NULL); DMM_deinit_IgnoreArg_handle();
    DMM_init_ExpectAndReturn(NULL, g_mock_dmm_handle); DMM_init_IgnoreArg_config();

    // Use the advancing time mock to simulate timeout
    // Time will advance by 200ms on each call, causing timeout after ~6 calls
    SYSTEM_get_tick_StubWithCallback(mock_system_get_tick_advancing);

    // DMM_read_voltage should return false (not ready) for all calls during timeout
    DMM_read_voltage_IgnoreAndReturn(false);

    inject_usb_command("READ:VOLT:DC?\n");

    // Act
    protocol_task();

    // Assert - Should generate SCPI timeout error
    clear_captured_response();
    inject_usb_command("SYST:ERR?\n");

    USB_task_Expect(g_mock_usb_handle);
    USB_rx_ready_StubWithCallback(mock_usb_rx_ready_check);
    USB_read_StubWithCallback(mock_usb_read_inject);
    USB_write_StubWithCallback(mock_usb_write_capture);

    protocol_task();

    const char *error_response = get_captured_response();
    TEST_ASSERT_TRUE(strlen(error_response) > 0);
    // The response string should not start with "0,", indicating no error
    TEST_ASSERT_NOT_EQUAL(0, strncmp(error_response, "0,", 2));
}

void test_multiple_commands_in_sequence(void)
{
    // Arrange
    USB_init_ExpectAndReturn(0, NULL, g_mock_usb_handle); USB_init_IgnoreArg_rx_buffer();
    USB_set_rx_callback_Ignore();
    protocol_init();

    // Send multiple commands
    USB_task_Expect(g_mock_usb_handle);
    USB_rx_ready_StubWithCallback(mock_usb_rx_ready_check);
    USB_read_StubWithCallback(mock_usb_read_inject);
    USB_write_StubWithCallback(mock_usb_write_capture);

    inject_usb_command("*CLS\n*IDN?\n");

    // Act
    protocol_task();

    // Assert
    const char *response = get_captured_response();
    TEST_ASSERT_TRUE(strlen(response) > 0);
    TEST_ASSERT_TRUE(strstr(response, "FOSSASIA") != NULL);
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

    // Configure and initiate to create state
    USB_task_Expect(g_mock_usb_handle);
    USB_rx_ready_StubWithCallback(mock_usb_rx_ready_check);
    USB_read_StubWithCallback(mock_usb_read_inject);
    USB_write_StubWithCallback(mock_usb_write_capture);

    DMM_init_ExpectAndReturn(NULL, g_mock_dmm_handle); DMM_init_IgnoreArg_config();
    DMM_deinit_Expect(NULL); DMM_deinit_IgnoreArg_handle();
    inject_usb_command("CONF:VOLT:DC 3\n");
    protocol_task();

    clear_captured_response();

    USB_task_Expect(g_mock_usb_handle);
    USB_rx_ready_StubWithCallback(mock_usb_rx_ready_check);
    USB_read_StubWithCallback(mock_usb_read_inject);
    USB_write_StubWithCallback(mock_usb_write_capture);

    DMM_deinit_Expect(NULL); DMM_deinit_IgnoreArg_handle();
    DMM_init_ExpectAndReturn(NULL, g_mock_dmm_handle); DMM_init_IgnoreArg_config();
    inject_usb_command("INIT:VOLT:DC\n");
    protocol_task();

    clear_captured_response();

    // Now reset and verify state is cleared
    USB_task_Expect(g_mock_usb_handle);
    USB_rx_ready_StubWithCallback(mock_usb_rx_ready_check);
    USB_read_StubWithCallback(mock_usb_read_inject);
    USB_write_StubWithCallback(mock_usb_write_capture);

    DMM_deinit_Expect(NULL); DMM_deinit_IgnoreArg_handle(); // Should deinit existing handle
    inject_usb_command("*RST\n");

    // Act
    protocol_task();

    // Assert - Try to fetch without re-initializing, should fail
    clear_captured_response();

    USB_task_Expect(g_mock_usb_handle);
    USB_rx_ready_StubWithCallback(mock_usb_rx_ready_check);
    USB_read_StubWithCallback(mock_usb_read_inject);
    USB_write_StubWithCallback(mock_usb_write_capture);

    inject_usb_command("FETC:VOLT:DC?\n");
    protocol_task();

    // Should generate error
    clear_captured_response();
    inject_usb_command("SYST:ERR?\n");

    USB_task_Expect(g_mock_usb_handle);
    USB_rx_ready_StubWithCallback(mock_usb_rx_ready_check);
    USB_read_StubWithCallback(mock_usb_read_inject);
    USB_write_StubWithCallback(mock_usb_write_capture);

    protocol_task();

    const char *error_response = get_captured_response();
    TEST_ASSERT_TRUE(strlen(error_response) > 0);
    // The response string should not start with "0,", indicating no error
    TEST_ASSERT_NOT_EQUAL(0, strncmp(error_response, "0,", 2));
}