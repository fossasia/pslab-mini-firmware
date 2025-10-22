/**
 * @file test_protocol_dso.c
 * @brief Unit tests for DSO-specific SCPI protocol functionality
 *
 * This file contains unit tests for the DSO protocol functionality covering:
 * - DSO SCPI command processing (CONF:OSC:*, INIT:OSC, FETC:OSC:*, etc.)
 * - DSO configuration validation (channel, timebase, buffer size)
 * - DSO error handling and recovery
 * - DSO state management and acquisition control
 * - DSO data retrieval and format validation
 *
 * The DSO protocol implements SCPI commands for oscilloscope functionality.
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
#include "mock_dso.h"
#include "mock_system.h"
#include "scpi_test_helpers.h"

#include "util/error.h"
#include "util/fixed_point.h"
#include "util/util.h"

#include "application/protocol.h"

// External declaration for the DSO completion callback
extern void dso_complete_callback(void);

// Define global variables required by scpi_test_helpers
char g_scpi_test_captured_response[SCPI_TEST_RESPONSE_BUFFER_SIZE];
size_t g_scpi_test_captured_response_len;
char g_scpi_test_injected_data[SCPI_TEST_USB_BUFFER_SIZE];
size_t g_scpi_test_injected_data_len;

// Test fixtures and mock data
static USB_Handle *g_mock_usb_handle;
static DSO_Handle *g_mock_dso_handle;
static CircularBuffer g_mock_usb_rx_buffer;
static uint8_t g_mock_usb_rx_data[SCPI_TEST_USB_BUFFER_SIZE];
static uint32_t g_mock_system_tick;
static int g_mock_system_tick_call_count;

void setUp(void)
{
    // Initialize test fixtures
    g_mock_usb_handle = (USB_Handle *)0x12345678; // Mock handle
    g_mock_dso_handle = (DSO_Handle *)0x13579BDF; // Mock handle
    g_scpi_test_captured_response_len = 0;
    g_mock_system_tick = 1000; // Start at 1 second
    g_scpi_test_injected_data_len = 0;
    g_mock_system_tick_call_count = 0;

    // Clear buffers
    scpi_clear_captured_response();
    memset(g_scpi_test_injected_data, 0, sizeof(g_scpi_test_injected_data));

    // Initialize circular buffer for USB RX simulation
    circular_buffer_init(&g_mock_usb_rx_buffer, g_mock_usb_rx_data, SCPI_TEST_USB_BUFFER_SIZE);

    // Initialize mocks
    mock_usb_Init();
    mock_dso_Init();
    mock_system_Init();
}

void tearDown(void)
{
    // Clean up protocol if initialized
    if (protocol_is_initialized()) {
        // Set up mock expectations for cleanup - DSO reset might be called
        USB_deinit_Ignore();
        DSO_stop_Ignore();
        DSO_deinit_Ignore();
        protocol_deinit();
    }

    // Clean up mocks
    mock_usb_Destroy();
    mock_dso_Destroy();
    mock_system_Destroy();
}

// ============================================================================
// Test Helper Functions
// ============================================================================

/**
 * @brief Mock SYSTEM_get_tick implementation
 */
static uint32_t mock_system_get_tick_impl(int cmock_num_calls)
{
    (void)cmock_num_calls;
    g_mock_system_tick_call_count++;

    // Simulate acquisition completion after a few calls (simulating time passing)
    if (g_mock_system_tick_call_count == 3) {
        dso_complete_callback();
    }

    // Advance time slightly each call to simulate passage of time
    g_mock_system_tick += 10;
    return g_mock_system_tick;
}

/**
 * @brief Mock SYSTEM_get_tick implementation that completes immediately
 */
static uint32_t mock_system_get_tick_immediate_completion(int cmock_num_calls)
{
    (void)cmock_num_calls;
    // Complete acquisition on first call
    dso_complete_callback();
    return g_mock_system_tick;
}

/**
 * @brief Simulate DSO acquisition completion by calling the completion callback directly
 *
 * This function simulates the hardware DSO completing an acquisition by
 * calling the dso_complete_callback function directly. This sets the proper
 * acquisition state flags in the DSO module.
 */
static void simulate_dso_acquisition_completion(void)
{
    dso_complete_callback();
}

/**
 * @brief Helper to initialize protocol for DSO tests
 */
static void setup_protocol_for_dso_test(void)
{
    USB_init_ExpectAndReturn(0, NULL, g_mock_usb_handle);
    USB_init_IgnoreArg_rx_buffer();
    USB_set_rx_callback_Ignore();
    protocol_init();
}

// ============================================================================
// SCPI Command Tests - DSO Configuration Commands
// ============================================================================

void test_scpi_configure_oscilloscope_channel_ch1(void)
{
    // Arrange
    setup_protocol_for_dso_test();

    // Mock DSO configuration success
    DSO_init_ExpectAndReturn(NULL, g_mock_dso_handle);
    DSO_init_IgnoreArg_config();
    DSO_get_max_sample_rate_ExpectAndReturn(DSO_MODE_SINGLE_CHANNEL, 2000000);

    // Act
    scpi_inject_usb_command("CONFigure:OSCilloscope:CHANnel CH1\n");
    scpi_run_protocol_with_usb_mocks(g_mock_usb_handle);

    // Assert
    char const *response = scpi_get_captured_response();
    TEST_ASSERT_EQUAL(0, strlen(response)); // CONF command generates no response
}

void test_scpi_configure_oscilloscope_channel_ch2(void)
{
    // Arrange
    setup_protocol_for_dso_test();

    DSO_init_ExpectAndReturn(NULL, g_mock_dso_handle);
    DSO_init_IgnoreArg_config();
    DSO_get_max_sample_rate_ExpectAndReturn(DSO_MODE_SINGLE_CHANNEL, 2000000);

    // Act
    scpi_inject_usb_command("CONF:OSC:CHAN CH2\n");
    scpi_run_protocol_with_usb_mocks(g_mock_usb_handle);

    // Assert
    char const *response = scpi_get_captured_response();
    TEST_ASSERT_EQUAL(0, strlen(response)); // CONF command generates no response
}

void test_scpi_configure_oscilloscope_channel_dual(void)
{
    // Arrange
    setup_protocol_for_dso_test();

    DSO_init_ExpectAndReturn(NULL, g_mock_dso_handle);
    DSO_init_IgnoreArg_config();
    DSO_get_max_sample_rate_ExpectAndReturn(DSO_MODE_DUAL_CHANNEL, 2000000);

    // Act
    scpi_inject_usb_command("CONF:OSC:CHAN CH1CH2\n");
    scpi_run_protocol_with_usb_mocks(g_mock_usb_handle);

    // Assert
    char const *response = scpi_get_captured_response();
    TEST_ASSERT_EQUAL(0, strlen(response)); // CONF command generates no response
}

void test_scpi_configure_oscilloscope_channel_invalid(void)
{
    // Arrange
    setup_protocol_for_dso_test();

    // Act
    scpi_inject_usb_command("CONF:OSC:CHAN INVALID\n");
    scpi_inject_usb_command("SYST:ERR?\n");
    scpi_run_protocol_with_usb_mocks(g_mock_usb_handle);

    // Assert - Should generate SCPI error
    TEST_ASSERT_SCPI_ERROR(scpi_get_captured_response());
}

void test_scpi_configure_oscilloscope_channel_query(void)
{
    // Arrange
    setup_protocol_for_dso_test();

    // Configure CH1 first
    DSO_init_ExpectAndReturn(NULL, g_mock_dso_handle);
    DSO_init_IgnoreArg_config();
    DSO_get_max_sample_rate_ExpectAndReturn(DSO_MODE_SINGLE_CHANNEL, 2000000);

    // Mock the get_config call to return CH1 configuration
    DSO_Config mock_config = {
        .mode = DSO_MODE_SINGLE_CHANNEL,
        .channel = DSO_CHANNEL_0,
        .sample_rate = 1000000,
        .buffer = NULL,
        .buffer_size = 512,
        .complete_callback = NULL
    };
    DSO_get_config_ExpectAndReturn(g_mock_dso_handle, mock_config);

    // Act
    scpi_inject_usb_command("CONF:OSC:CHAN CH1\n");
    scpi_inject_usb_command("CONF:OSC:CHAN?\n");
    scpi_run_protocol_with_usb_mocks(g_mock_usb_handle);

    // Assert
    char const *response = scpi_get_captured_response();
    TEST_ASSERT_TRUE(strlen(response) > 0);
    TEST_ASSERT_TRUE(strstr(response, "CH1") != NULL);
}

void test_scpi_configure_oscilloscope_timebase(void)
{
    // Arrange
    setup_protocol_for_dso_test();

    DSO_init_ExpectAndReturn(NULL, g_mock_dso_handle);
    DSO_init_IgnoreArg_config();
    DSO_get_max_sample_rate_ExpectAndReturn(DSO_MODE_SINGLE_CHANNEL, 2000000);

    // Act
    scpi_inject_usb_command("CONF:OSC:TIME 500\n"); // 500 µs/div
    scpi_run_protocol_with_usb_mocks(g_mock_usb_handle);

    // Assert
    char const *response = scpi_get_captured_response();
    TEST_ASSERT_EQUAL(0, strlen(response)); // CONF command generates no response
}

void test_scpi_configure_oscilloscope_timebase_query(void)
{
    // Arrange
    setup_protocol_for_dso_test();

    // Configure timebase first
    DSO_init_ExpectAndReturn(NULL, g_mock_dso_handle);
    DSO_init_IgnoreArg_config();
    DSO_get_max_sample_rate_ExpectAndReturn(DSO_MODE_SINGLE_CHANNEL, 2000000);

    // Act
    scpi_inject_usb_command("CONF:OSC:TIME 200\n");
    scpi_inject_usb_command("CONF:OSC:TIME?\n");
    scpi_run_protocol_with_usb_mocks(g_mock_usb_handle);

    // Assert
    char const *response = scpi_get_captured_response();
    TEST_ASSERT_TRUE(strlen(response) > 0);
    TEST_ASSERT_TRUE(strstr(response, "200") != NULL);
}

void test_scpi_configure_oscilloscope_acquire_points(void)
{
    // Arrange
    setup_protocol_for_dso_test();

    DSO_init_ExpectAndReturn(NULL, g_mock_dso_handle);
    DSO_init_IgnoreArg_config();
    DSO_get_max_sample_rate_ExpectAndReturn(DSO_MODE_SINGLE_CHANNEL, 2000000);

    // Act
    scpi_inject_usb_command("CONF:OSC:ACQ:POIN 1024\n");
    scpi_run_protocol_with_usb_mocks(g_mock_usb_handle);

    // Assert
    char const *response = scpi_get_captured_response();
    TEST_ASSERT_EQUAL(0, strlen(response)); // CONF command generates no response
}

void test_scpi_configure_oscilloscope_acquire_points_query(void)
{
    // Arrange
    setup_protocol_for_dso_test();

    // Configure points first
    DSO_init_ExpectAndReturn(NULL, g_mock_dso_handle);
    DSO_init_IgnoreArg_config();
    DSO_get_max_sample_rate_ExpectAndReturn(DSO_MODE_SINGLE_CHANNEL, 3000000);

    // Act
    scpi_inject_usb_command("CONF:OSC:ACQ:POIN 2048\n");
    scpi_inject_usb_command("CONF:OSC:ACQ:POIN?\n");
    scpi_run_protocol_with_usb_mocks(g_mock_usb_handle);

    // Assert
    char const *response = scpi_get_captured_response();
    TEST_ASSERT_TRUE(strlen(response) > 0);
    TEST_ASSERT_TRUE(strstr(response, "2048") != NULL);
}

void test_scpi_configure_oscilloscope_acquire_srate_query(void)
{
    // Arrange
    setup_protocol_for_dso_test();

    // Configure DSO first
    DSO_init_ExpectAndReturn(NULL, g_mock_dso_handle);
    DSO_init_IgnoreArg_config();
    DSO_get_max_sample_rate_ExpectAndReturn(DSO_MODE_SINGLE_CHANNEL, 2000000);

    // Mock the get_config call to return sample rate
    DSO_Config mock_config = {
        .mode = DSO_MODE_SINGLE_CHANNEL,
        .channel = DSO_CHANNEL_0,
        .sample_rate = 1500000,
        .buffer = NULL,
        .buffer_size = 512,
        .complete_callback = NULL
    };
    DSO_get_config_ExpectAndReturn(g_mock_dso_handle, mock_config);

    // Act
    scpi_inject_usb_command("CONF:OSC:CHAN CH1\n");
    scpi_inject_usb_command("CONF:OSC:ACQ:SRAT?\n");
    scpi_run_protocol_with_usb_mocks(g_mock_usb_handle);

    // Assert
    char const *response = scpi_get_captured_response();
    TEST_ASSERT_TRUE(strlen(response) > 0);
    TEST_ASSERT_TRUE(strstr(response, "1500000") != NULL);
}

// ============================================================================
// SCPI Command Tests - DSO Operation Commands
// ============================================================================

void test_scpi_initiate_oscilloscope_success(void)
{
    // Arrange
    setup_protocol_for_dso_test();

    // Configure DSO first
    DSO_init_ExpectAndReturn(NULL, g_mock_dso_handle);
    DSO_init_IgnoreArg_config();
    DSO_get_max_sample_rate_ExpectAndReturn(DSO_MODE_SINGLE_CHANNEL, 2000000);
    DSO_start_Expect(g_mock_dso_handle);

    // Act
    scpi_inject_usb_command("CONF:OSC:CHAN CH1\n");
    scpi_inject_usb_command("INIT:OSC\n");
    scpi_run_protocol_with_usb_mocks(g_mock_usb_handle);

    // Assert
    char const *response = scpi_get_captured_response();
    TEST_ASSERT_EQUAL(0, strlen(response)); // INIT command generates no response
}

void test_scpi_initiate_oscilloscope_not_configured(void)
{
    // Arrange
    setup_protocol_for_dso_test();
    DSO_get_max_sample_rate_ExpectAndReturn(DSO_MODE_SINGLE_CHANNEL, 2000000);
    DSO_init_ExpectAndReturn(NULL, g_mock_dso_handle);
    DSO_init_IgnoreArg_config();
    DSO_start_Expect(g_mock_dso_handle);

    // Act
    scpi_inject_usb_command("INIT:OSC\n");
    scpi_inject_usb_command("SYST:ERR?\n");
    scpi_run_protocol_with_usb_mocks(g_mock_usb_handle);

    // Assert - Should not generate SCPI error
    TEST_ASSERT_SCPI_NO_ERROR(scpi_get_captured_response());
}

void test_scpi_fetch_oscilloscope_data_success(void)
{
    // Arrange
    setup_protocol_for_dso_test();

    // Configure and initiate DSO first
    DSO_get_max_sample_rate_ExpectAndReturn(DSO_MODE_SINGLE_CHANNEL, 2000000);
    DSO_init_ExpectAndReturn(NULL, g_mock_dso_handle);
    DSO_init_IgnoreArg_config();
    DSO_start_Expect(g_mock_dso_handle);
    // Simulate acquisition in progress for one tick
    // This is necessary for the tick mock to be called, which sets the acquisition complete flag
    DSO_is_acquisition_in_progress_ExpectAndReturn(g_mock_dso_handle, true);
    // Simulate acquisition complete
    DSO_is_acquisition_in_progress_ExpectAndReturn(g_mock_dso_handle, false);
    DSO_stop_Expect(g_mock_dso_handle);

    // Set up SYSTEM_get_tick mock to complete acquisition immediately when polled
    SYSTEM_get_tick_StubWithCallback(mock_system_get_tick_immediate_completion);

    // Act
    scpi_inject_usb_command("CONF:OSC:CHAN CH1\n");
    scpi_inject_usb_command("INIT:OSC\n");
    scpi_inject_usb_command("FETCH:OSC:DATA?\n");

    scpi_run_protocol_with_usb_mocks(g_mock_usb_handle);

    // Assert
    char const *response = scpi_get_captured_response();
    TEST_ASSERT_TRUE(strlen(response) > 0);
    // Should contain binary data block marker
    TEST_ASSERT_TRUE(response[0] == '#');
}

void test_scpi_fetch_oscilloscope_data_not_initiated(void)
{
    // Arrange
    setup_protocol_for_dso_test();

    // Act
    scpi_inject_usb_command("FETC:OSC:DATA?\n");
    scpi_inject_usb_command("SYST:ERR?\n");
    scpi_run_protocol_with_usb_mocks(g_mock_usb_handle);

    // Assert - Should generate SCPI error
    TEST_ASSERT_SCPI_ERROR(scpi_get_captured_response());
}

void test_scpi_read_oscilloscope_complete_flow(void)
{
    // Arrange
    setup_protocol_for_dso_test();

    // Mock the complete READ flow (INIT + FETCH)
    DSO_get_max_sample_rate_ExpectAndReturn(DSO_MODE_SINGLE_CHANNEL, 2000000);
    // READ checks for acquisition in progress and aborts if true
    DSO_is_acquisition_in_progress_ExpectAndReturn(g_mock_dso_handle, false);
    DSO_init_ExpectAndReturn(NULL, g_mock_dso_handle);
    DSO_init_IgnoreArg_config();
    DSO_start_Expect(g_mock_dso_handle);
    // Simulate acquisition in progress for three ticks
    DSO_is_acquisition_in_progress_ExpectAndReturn(g_mock_dso_handle, true);
    DSO_is_acquisition_in_progress_ExpectAndReturn(g_mock_dso_handle, true);
    DSO_is_acquisition_in_progress_ExpectAndReturn(g_mock_dso_handle, true);
    // Simulate acquisition complete
    DSO_is_acquisition_in_progress_ExpectAndReturn(g_mock_dso_handle, false);
    DSO_stop_Expect(g_mock_dso_handle);

    // Set up SYSTEM_get_tick mock to complete acquisition after a few calls
    SYSTEM_get_tick_StubWithCallback(mock_system_get_tick_impl);

    // Act
    scpi_inject_usb_command("CONF:OSC:CHAN CH2\n");
    scpi_inject_usb_command("READ:OSC?\n");
    scpi_run_protocol_with_usb_mocks(g_mock_usb_handle);

    // Assert
    char const *response = scpi_get_captured_response();
    TEST_ASSERT_TRUE(strlen(response) > 0);
    // Should contain binary data block marker
    TEST_ASSERT_TRUE(response[0] == '#');
}

void test_scpi_measure_oscilloscope_complete_flow(void)
{
    // Arrange
    setup_protocol_for_dso_test();

    // Mock the complete MEASURE flow (CONF + READ)
    DSO_get_max_sample_rate_ExpectAndReturn(DSO_MODE_DUAL_CHANNEL, 2000000);
    DSO_is_acquisition_in_progress_ExpectAndReturn(g_mock_dso_handle, false);
    DSO_init_ExpectAndReturn(NULL, g_mock_dso_handle);
    DSO_init_IgnoreArg_config();
    DSO_start_Expect(g_mock_dso_handle);
    DSO_is_acquisition_in_progress_ExpectAndReturn(g_mock_dso_handle, true);
    DSO_is_acquisition_in_progress_ExpectAndReturn(g_mock_dso_handle, true);
    DSO_is_acquisition_in_progress_ExpectAndReturn(g_mock_dso_handle, true);
    DSO_is_acquisition_in_progress_ExpectAndReturn(g_mock_dso_handle, false);
    DSO_stop_Expect(g_mock_dso_handle);


    // Set up SYSTEM_get_tick mock to complete acquisition after a few calls
    SYSTEM_get_tick_StubWithCallback(mock_system_get_tick_impl);

    // Act
    scpi_inject_usb_command("MEAS:OSC? CH1CH2\n");
    scpi_run_protocol_with_usb_mocks(g_mock_usb_handle);

    // Assert
    char const *response = scpi_get_captured_response();
    TEST_ASSERT_TRUE(strlen(response) > 0);
    // Should contain binary data block marker
    TEST_ASSERT_TRUE(response[0] == '#');
}

void test_scpi_abort_oscilloscope(void)
{
    // Arrange
    setup_protocol_for_dso_test();

    // Configure and start acquisition first
    DSO_init_ExpectAndReturn(NULL, g_mock_dso_handle);
    DSO_init_IgnoreArg_config();
    DSO_get_max_sample_rate_ExpectAndReturn(DSO_MODE_SINGLE_CHANNEL, 2000000);
    DSO_start_Expect(g_mock_dso_handle);
    DSO_stop_Expect(g_mock_dso_handle);
    DSO_deinit_Expect(g_mock_dso_handle);

    // Act
    scpi_inject_usb_command("CONF:OSC:CHAN CH1\n");
    scpi_inject_usb_command("INIT:OSC\n");
    scpi_inject_usb_command("ABOR:OSC\n");
    scpi_run_protocol_with_usb_mocks(g_mock_usb_handle);

    // Assert
    char const *response = scpi_get_captured_response();
    TEST_ASSERT_EQUAL(0, strlen(response)); // ABORT command generates no response
}

void test_scpi_status_oscilloscope_acquisition_query_not_started(void)
{
    // Arrange
    setup_protocol_for_dso_test();

    // Test initial state - no acquisition started
    scpi_inject_usb_command("STAT:OSC:ACQ?\n");
    scpi_run_protocol_with_usb_mocks(g_mock_usb_handle);

    // Assert
    char const *response = scpi_get_captured_response();
    TEST_ASSERT_TRUE(strlen(response) > 0);
    TEST_ASSERT_TRUE(strstr(response, "0") != NULL); // Should be "0" for not started
}

void test_scpi_status_oscilloscope_acquisition_query_running_complete(void)
{
    // Arrange
    setup_protocol_for_dso_test();

    // Configure and start acquisition
    DSO_init_ExpectAndReturn(NULL, g_mock_dso_handle);
    DSO_init_IgnoreArg_config();
    DSO_get_max_sample_rate_ExpectAndReturn(DSO_MODE_SINGLE_CHANNEL, 2000000);
    DSO_start_Expect(g_mock_dso_handle);
    DSO_is_acquisition_in_progress_ExpectAndReturn(g_mock_dso_handle, true);

    // Act
    scpi_inject_usb_command("CONF:OSC:CHAN CH1\n");
    scpi_inject_usb_command("INIT:OSC\n");
    scpi_inject_usb_command("STAT:OSC:ACQ?\n");
    scpi_run_protocol_with_usb_mocks(g_mock_usb_handle);

    // Assert - should be "1" for acquisition in progress
    char const *response = scpi_get_captured_response();
    TEST_ASSERT_TRUE(strlen(response) > 0);
    TEST_ASSERT_TRUE(strstr(response, "1") != NULL);

    // Arrange - Complete acquisition and test completed state
    simulate_dso_acquisition_completion();
    DSO_is_acquisition_in_progress_ExpectAndReturn(g_mock_dso_handle, false);

    // Act
    scpi_inject_usb_command("STAT:OSC:ACQ?\n");
    scpi_run_protocol_with_usb_mocks(g_mock_usb_handle);

    // Assert - should be "2" for acquisition complete
    char const *response2 = scpi_get_captured_response();
    TEST_ASSERT_TRUE(strlen(response2) > 0);
    TEST_ASSERT_TRUE(strstr(response2, "2") != NULL);
}

// ============================================================================
// DSO Error Handling Tests
// ============================================================================

void test_dso_configuration_invalid_sample_rate(void)
{
    // Arrange
    setup_protocol_for_dso_test();

    // Mock sample rate validation failure
    DSO_init_ExpectAndThrow(NULL, ERROR_INVALID_ARGUMENT);
    DSO_init_IgnoreArg_config();
    DSO_get_max_sample_rate_ExpectAndReturn(DSO_MODE_SINGLE_CHANNEL, 1000000);

    // Act
    scpi_inject_usb_command("CONF:OSC:TIME 1\n"); // Very fast timebase causing invalid sample rate
    scpi_inject_usb_command("SYST:ERR?\n");
    scpi_run_protocol_with_usb_mocks(g_mock_usb_handle);

    // Assert - Should generate SCPI error
    TEST_ASSERT_SCPI_ERROR(scpi_get_captured_response());
}

void test_dso_start_failure(void)
{
    // Arrange
    setup_protocol_for_dso_test();

    // Configure DSO first
    DSO_init_ExpectAndReturn(NULL, g_mock_dso_handle);
    DSO_init_IgnoreArg_config();
    DSO_get_max_sample_rate_ExpectAndReturn(DSO_MODE_SINGLE_CHANNEL, 2000000);

    // Mock DSO start failure
    DSO_start_ExpectAndThrow(g_mock_dso_handle, ERROR_HARDWARE_FAULT);

    // Act
    scpi_inject_usb_command("CONF:OSC:CHAN CH1\n");
    scpi_inject_usb_command("INIT:OSC\n");
    scpi_inject_usb_command("SYST:ERR?\n");
    scpi_run_protocol_with_usb_mocks(g_mock_usb_handle);

    // Assert - Should generate SCPI error
    TEST_ASSERT_SCPI_ERROR(scpi_get_captured_response());
}

void test_dso_configuration_during_acquisition(void)
{
    // Arrange
    setup_protocol_for_dso_test();

    // Configure and start acquisition
    DSO_init_ExpectAndReturn(NULL, g_mock_dso_handle);
    DSO_init_IgnoreArg_config();
    DSO_get_max_sample_rate_ExpectAndReturn(DSO_MODE_SINGLE_CHANNEL, 2000000);
    DSO_start_Expect(g_mock_dso_handle);
    DSO_is_acquisition_in_progress_ExpectAndReturn(g_mock_dso_handle, true);

    // Act
    scpi_inject_usb_command("CONF:OSC:CHAN CH1\n");
    scpi_inject_usb_command("INIT:OSC\n");
    scpi_inject_usb_command("CONF:OSC:TIME 300\n");
    scpi_inject_usb_command("SYST:ERR?\n");
    scpi_run_protocol_with_usb_mocks(g_mock_usb_handle);

    // Assert - Should generate SCPI error for configuration during acquisition
    TEST_ASSERT_SCPI_ERROR(scpi_get_captured_response());
}
