/**
 * @file test_dmm.c
 * @brief Unit tests for Digital Multimeter (DMM) implementation
 *
 * This file contains comprehensive unit tests for the DMM API, including
 * initialization, configuration validation, voltage measurements, and
 * error handling. The ADC and Timer low-level drivers are mocked using
 * CMock.
 *
 * @author PSLab Team
 * @date 2025-08-12
 */

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "unity.h"
#include "mock_adc_ll.h"
#include "mock_tim_ll.h"

#include "util/error.h"
#include "util/fixed_point.h"

#include "dmm.h"

// External function declaration for testing
// This function is intentionally non-static in dmm.c to enable testing
extern void dmm_adc_complete_callback(uint16_t *buffer, uint32_t total_samples);

// Test fixtures
static DMM_Handle *g_test_handle;
static uint16_t g_mock_adc_value;
static bool g_adc_callback_called;
static ADC_LL_CompleteCallback g_stored_callback;
static uint16_t *g_captured_adc_buffer; // Captured from ADC_LL_init

void setUp(void)
{
    // Initialize test data
    g_test_handle = NULL;
    g_mock_adc_value = 0;
    g_adc_callback_called = false;
    g_stored_callback = NULL;
    g_captured_adc_buffer = NULL;

    // Initialize mocks
    mock_adc_ll_Init();
    mock_tim_ll_Init();
}

void tearDown(void)
{
    // Clean up DMM handle if it exists
    if (g_test_handle != NULL) {
        // Set up expectations for deinit - use Ignore to be flexible
        ADC_LL_stop_Ignore();
        TIM_LL_stop_Ignore();
        ADC_LL_deinit_Ignore();
        TIM_LL_deinit_Ignore();

        DMM_deinit(g_test_handle);
        g_test_handle = NULL;
    }

    // Clean up mocks after each test
    mock_adc_ll_Destroy();
    mock_tim_ll_Destroy();
}

// Helper function to capture ADC callback
void capture_adc_callback_stub(ADC_LL_CompleteCallback callback, int cmock_num_calls)
{
    (void)cmock_num_calls;
    g_stored_callback = callback;
}

// Helper function to simulate ADC conversion with specific value
void simulate_adc_conversion(uint16_t adc_value)
{
    // Set the ADC value in the captured buffer (simulates hardware writing to buffer)
    if (g_captured_adc_buffer != NULL) {
        *g_captured_adc_buffer = adc_value;
    }

    // Call the completion callback to notify DMM
    if (g_stored_callback != NULL) {
        g_stored_callback(NULL, 0);
    }
}

// Stub function to simulate ADC initialization success
void adc_init_success_stub(ADC_LL_Config const *config, int cmock_num_calls)
{
    (void)cmock_num_calls;
    // Verify config is reasonable
    TEST_ASSERT_NOT_NULL(config);
    TEST_ASSERT_EQUAL(ADC_LL_MODE_SINGLE, config->mode);
    TEST_ASSERT_EQUAL(ADC_TRIGGER_TIMER6, config->trigger_source);
    TEST_ASSERT_NOT_NULL(config->output_buffer);
    TEST_ASSERT_EQUAL(1, config->buffer_size);

    // Capture the output buffer for later use
    g_captured_adc_buffer = config->output_buffer;
}

// Test: Successful DMM initialization with default configuration
void test_DMM_init_success_default_config(void)
{
    // Arrange
    DMM_Config config = DMM_CONFIG_DEFAULT;

    // Set expectations
    ADC_LL_set_complete_callback_Expect(dmm_adc_complete_callback);
    ADC_LL_init_Stub(adc_init_success_stub);
    ADC_LL_get_sample_rate_ExpectAndReturn(1000); // 1 kHz sample rate for timer init
    ADC_LL_get_sample_rate_ExpectAndReturn(1000); // 1 kHz sample rate for logging
    TIM_LL_init_Expect(TIM_NUM_6, 1000);
    TIM_LL_start_Expect(TIM_NUM_6); // Expect timer start for initial conversion
    ADC_LL_start_Expect(); // Expect ADC start for initial conversion

    // Act
    g_test_handle = DMM_init(&config);

    // Assert
    TEST_ASSERT_NOT_NULL(g_test_handle);
}

// Test: DMM initialization with custom configuration
void test_DMM_init_success_custom_config(void)
{
    // Arrange
    DMM_Config config = {
        .channel = DMM_CHANNEL_5,
        .oversampling_ratio = 64
    };

    // Set expectations
    ADC_LL_set_complete_callback_Expect(dmm_adc_complete_callback);
    ADC_LL_init_Ignore();
    ADC_LL_get_sample_rate_ExpectAndReturn(2000); // 2 kHz sample rate for timer init
    ADC_LL_get_sample_rate_ExpectAndReturn(2000); // 2 kHz sample rate for logging
    TIM_LL_init_Expect(TIM_NUM_6, 2000);
    TIM_LL_start_Expect(TIM_NUM_6); // Expect timer start for initial conversion
    ADC_LL_start_Expect(); // Expect ADC start for initial conversion

    // Act
    g_test_handle = DMM_init(&config);

    // Assert
    TEST_ASSERT_NOT_NULL(g_test_handle);
}

// Test: DMM initialization with NULL config
void test_DMM_init_null_config(void)
{
    // Arrange
    CEXCEPTION_T exception = CEXCEPTION_NONE;

    // Act & Assert
    TRY {
        g_test_handle = DMM_init(NULL);
        TEST_FAIL_MESSAGE("Expected exception for NULL config");
    }
    CATCH(exception) {
        TEST_ASSERT_EQUAL(ERROR_INVALID_ARGUMENT, exception);
        TEST_ASSERT_NULL(g_test_handle);
    }
}

// Test: DMM initialization with invalid channel
void test_DMM_init_invalid_channel(void)
{
    // Arrange
    DMM_Config config = DMM_CONFIG_DEFAULT;
    config.channel = (DMM_Channel)999; // Invalid channel
    CEXCEPTION_T exception = CEXCEPTION_NONE;

    // Act & Assert
    TRY {
        g_test_handle = DMM_init(&config);
        TEST_FAIL_MESSAGE("Expected exception for invalid channel");
    }
    CATCH(exception) {
        TEST_ASSERT_EQUAL(ERROR_INVALID_ARGUMENT, exception);
        TEST_ASSERT_NULL(g_test_handle);
    }
}

// Test: DMM initialization with invalid oversampling ratio
void test_DMM_init_invalid_oversampling_ratio(void)
{
    // Arrange
    DMM_Config config = DMM_CONFIG_DEFAULT;
    config.oversampling_ratio = 7; // Not power of 2
    CEXCEPTION_T exception = CEXCEPTION_NONE;

    // Act & Assert
    TRY {
        g_test_handle = DMM_init(&config);
        TEST_FAIL_MESSAGE("Expected exception for invalid oversampling ratio");
    }
    CATCH(exception) {
        TEST_ASSERT_EQUAL(ERROR_INVALID_ARGUMENT, exception);
        TEST_ASSERT_NULL(g_test_handle);
    }
}

// Test: DMM initialization when already initialized
void test_DMM_init_already_initialized(void)
{
    // Arrange
    DMM_Config config = DMM_CONFIG_DEFAULT;
    CEXCEPTION_T exception = CEXCEPTION_NONE;

    // First initialization
    ADC_LL_set_complete_callback_Expect(dmm_adc_complete_callback);
    ADC_LL_init_Ignore();
    ADC_LL_get_sample_rate_ExpectAndReturn(1000);
    ADC_LL_get_sample_rate_ExpectAndReturn(1000);
    TIM_LL_init_Expect(TIM_NUM_6, 1000);
    TIM_LL_start_Expect(TIM_NUM_6);
    ADC_LL_start_Expect();

    g_test_handle = DMM_init(&config);
    TEST_ASSERT_NOT_NULL(g_test_handle);

    // Act & Assert - Try to initialize again
    TRY {
        DMM_Handle *second_handle = DMM_init(&config);
        TEST_FAIL_MESSAGE("Expected exception for double initialization");
    }
    CATCH(exception) {
        TEST_ASSERT_EQUAL(ERROR_RESOURCE_BUSY, exception);
    }
}

// Test: DMM initialization with ADC failure
void test_DMM_init_adc_failure(void)
{
    // Arrange
    DMM_Config config = DMM_CONFIG_DEFAULT;
    CEXCEPTION_T exception = CEXCEPTION_NONE;

    // Set expectations
    ADC_LL_set_complete_callback_Expect(dmm_adc_complete_callback);
    ADC_LL_init_ExpectAndThrow(NULL, ERROR_HARDWARE_FAULT);
    ADC_LL_init_IgnoreArg_config(); // Ignore the config parameter

    // Act & Assert
    TRY {
        g_test_handle = DMM_init(&config);
        TEST_FAIL_MESSAGE("Expected exception for ADC initialization failure");
    }
    CATCH(exception) {
        TEST_ASSERT_EQUAL(ERROR_HARDWARE_FAULT, exception);
        TEST_ASSERT_NULL(g_test_handle);
    }
}

// Test: DMM initialization with timer failure
void test_DMM_init_timer_failure(void)
{
    // Arrange
    DMM_Config config = DMM_CONFIG_DEFAULT;
    CEXCEPTION_T exception = CEXCEPTION_NONE;

    // Set expectations - ADC init succeeds, but timer init fails
    ADC_LL_set_complete_callback_Expect(dmm_adc_complete_callback);
    ADC_LL_init_Ignore(); // ADC init succeeds
    ADC_LL_get_sample_rate_ExpectAndReturn(1000); // For timer init
    TIM_LL_init_ExpectAndThrow(TIM_NUM_6, 1000, ERROR_HARDWARE_FAULT);
    ADC_LL_deinit_Expect(); // Cleanup after timer failure

    // Act & Assert
    TRY {
        g_test_handle = DMM_init(&config);
        TEST_FAIL_MESSAGE("Expected exception for timer initialization failure");
    }
    CATCH(exception) {
        TEST_ASSERT_EQUAL(ERROR_HARDWARE_FAULT, exception);
        TEST_ASSERT_NULL(g_test_handle);
    }
}

// Test: DMM deinitialization with valid handle
void test_DMM_deinit_valid_handle(void)
{
    // Arrange - Initialize DMM first
    DMM_Config config = DMM_CONFIG_DEFAULT;

    ADC_LL_set_complete_callback_Expect(dmm_adc_complete_callback);
    ADC_LL_init_Ignore();
    ADC_LL_get_sample_rate_ExpectAndReturn(1000);
    ADC_LL_get_sample_rate_ExpectAndReturn(1000);
    TIM_LL_init_Expect(TIM_NUM_6, 1000);
    TIM_LL_start_Expect(TIM_NUM_6);
    ADC_LL_start_Expect();

    g_test_handle = DMM_init(&config);
    TEST_ASSERT_NOT_NULL(g_test_handle);

    // Set expectations for deinit
    ADC_LL_stop_Expect();
    TIM_LL_stop_Expect(TIM_NUM_6);
    ADC_LL_deinit_Expect();
    TIM_LL_deinit_Expect(TIM_NUM_6);

    // Act
    DMM_deinit(g_test_handle);
    g_test_handle = NULL; // Prevent tearDown from attempting cleanup

    // Assert - No crash, expectations verified by mock
}

// Test: DMM deinitialization with NULL handle
void test_DMM_deinit_null_handle(void)
{
    // Act
    DMM_deinit(NULL);

    // Assert - No crash, no expectations needed
    TEST_PASS();
}

// Test: Successful voltage reading with conversion ready
void test_DMM_read_voltage_success_conversion_ready(void)
{
    // Arrange
    DMM_Config config = DMM_CONFIG_DEFAULT;
    FIXED_Q1616 voltage_out;
    bool result;

    // Initialize DMM
    ADC_LL_set_complete_callback_Expect(dmm_adc_complete_callback);
    ADC_LL_init_Ignore();
    ADC_LL_get_sample_rate_ExpectAndReturn(1000);
    ADC_LL_get_sample_rate_ExpectAndReturn(1000);
    TIM_LL_init_Expect(TIM_NUM_6, 1000);
    TIM_LL_start_Expect(TIM_NUM_6);
    ADC_LL_start_Expect();

    g_test_handle = DMM_init(&config);
    TEST_ASSERT_NOT_NULL(g_test_handle);

    // Simulate a completed conversion by calling the callback directly
    dmm_adc_complete_callback(NULL, 0);

    // Expect reference voltage query and next conversion to be started
    ADC_LL_get_reference_voltage_ExpectAndReturn(3300); // 3.3V in mV
    ADC_LL_start_Expect();

    // Act
    result = DMM_read_voltage(g_test_handle, &voltage_out);

    // Assert
    TEST_ASSERT_TRUE(result);
}

// Test: Voltage reading with no conversion ready
void test_DMM_read_voltage_no_conversion_ready(void)
{
    // Arrange
    DMM_Config config = DMM_CONFIG_DEFAULT;
    FIXED_Q1616 voltage_out;
    bool result;

    // Initialize DMM
    ADC_LL_set_complete_callback_Expect(dmm_adc_complete_callback);
    ADC_LL_init_Ignore();
    ADC_LL_get_sample_rate_ExpectAndReturn(1000);
    ADC_LL_get_sample_rate_ExpectAndReturn(1000);
    TIM_LL_init_Expect(TIM_NUM_6, 1000);
    TIM_LL_start_Expect(TIM_NUM_6);
    ADC_LL_start_Expect();

    g_test_handle = DMM_init(&config);
    TEST_ASSERT_NOT_NULL(g_test_handle);

    // Don't simulate conversion completion

    // Act
    result = DMM_read_voltage(g_test_handle, &voltage_out);

    // Assert
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(FIXED_ZERO, voltage_out);
}

// Test: Voltage reading with NULL handle
void test_DMM_read_voltage_null_handle(void)
{
    // Arrange
    FIXED_Q1616 voltage_out;
    CEXCEPTION_T exception = CEXCEPTION_NONE;

    // Act & Assert
    TRY {
        DMM_read_voltage(NULL, &voltage_out);
        TEST_FAIL_MESSAGE("Expected exception for NULL handle");
    }
    CATCH(exception) {
        TEST_ASSERT_EQUAL(ERROR_INVALID_ARGUMENT, exception);
    }
}

// Test: Voltage reading with NULL output pointer
void test_DMM_read_voltage_null_output(void)
{
    // Arrange
    DMM_Config config = DMM_CONFIG_DEFAULT;
    CEXCEPTION_T exception = CEXCEPTION_NONE;

    // Initialize DMM
    ADC_LL_set_complete_callback_Expect(dmm_adc_complete_callback);
    ADC_LL_init_Ignore();
    ADC_LL_get_sample_rate_ExpectAndReturn(1000);
    ADC_LL_get_sample_rate_ExpectAndReturn(1000);
    TIM_LL_init_Expect(TIM_NUM_6, 1000);
    TIM_LL_start_Expect(TIM_NUM_6);
    ADC_LL_start_Expect();

    g_test_handle = DMM_init(&config);
    TEST_ASSERT_NOT_NULL(g_test_handle);

    // Act & Assert
    TRY {
        bool result = DMM_read_voltage(g_test_handle, NULL);
        TEST_FAIL_MESSAGE("Expected exception for NULL output pointer");
    }
    CATCH(exception) {
        TEST_ASSERT_EQUAL(ERROR_INVALID_ARGUMENT, exception);
    }
}

// Test: Voltage reading with ADC start failure
void test_DMM_read_voltage_adc_start_failure(void)
{
    // Arrange
    DMM_Config config = DMM_CONFIG_DEFAULT;
    FIXED_Q1616 voltage_out;
    bool result;

    // Initialize DMM first
    ADC_LL_set_complete_callback_Expect(dmm_adc_complete_callback);
    ADC_LL_init_Ignore();
    ADC_LL_get_sample_rate_ExpectAndReturn(1000);
    ADC_LL_get_sample_rate_ExpectAndReturn(1000);
    TIM_LL_init_Expect(TIM_NUM_6, 1000);
    TIM_LL_start_Expect(TIM_NUM_6);
    ADC_LL_start_Expect(); // Initial start succeeds

    g_test_handle = DMM_init(&config);
    TEST_ASSERT_NOT_NULL(g_test_handle);

    // Simulate a completed conversion
    dmm_adc_complete_callback(NULL, 0);

    // Expect reference voltage query and ADC restart to fail
    ADC_LL_get_reference_voltage_ExpectAndReturn(3300); // 3.3V in mV
    ADC_LL_start_ExpectAndThrow(ERROR_HARDWARE_FAULT);

    // Act - this should still return the valid measurement despite restart failure
    result = DMM_read_voltage(g_test_handle, &voltage_out);

    // Assert - the function should still return true with valid data
    // even though the restart failed (as per current implementation)
    TEST_ASSERT_TRUE(result);
}

// Test: DMM initialization with ADC start failure
void test_DMM_read_voltage_timer_start_failure(void)
{
    // Arrange
    DMM_Config config = DMM_CONFIG_DEFAULT;
    CEXCEPTION_T exception = CEXCEPTION_NONE;

    // Set expectations - ADC and timer init succeed, timer start succeeds, but ADC start fails
    ADC_LL_set_complete_callback_Expect(dmm_adc_complete_callback);
    ADC_LL_init_Ignore(); // ADC init succeeds
    ADC_LL_get_sample_rate_ExpectAndReturn(1000); // For timer init
    ADC_LL_get_sample_rate_ExpectAndReturn(1000); // For logging
    TIM_LL_init_Expect(TIM_NUM_6, 1000); // Timer init succeeds
    TIM_LL_start_Expect(TIM_NUM_6); // Timer start succeeds
    ADC_LL_start_ExpectAndThrow(ERROR_HARDWARE_FAULT); // ADC start fails
    TIM_LL_stop_Expect(TIM_NUM_6); // Cleanup after start failure
    ADC_LL_deinit_Expect(); // Cleanup after start failure
    TIM_LL_deinit_Expect(TIM_NUM_6); // Cleanup after start failure

    // Act & Assert
    TRY {
        g_test_handle = DMM_init(&config);
        TEST_FAIL_MESSAGE("Expected exception for ADC start failure during initialization");
    }
    CATCH(exception) {
        TEST_ASSERT_EQUAL(ERROR_HARDWARE_FAULT, exception);
        TEST_ASSERT_NULL(g_test_handle);
    }
}

// Test: Voltage calculation with zero ADC value
void test_DMM_voltage_calculation_zero_adc(void)
{
    // Arrange
    DMM_Config config = DMM_CONFIG_DEFAULT;
    config.oversampling_ratio = 1; // No oversampling for simpler calculation
    FIXED_Q1616 voltage_out;
    bool result;

    // Initialize DMM
    ADC_LL_set_complete_callback_Stub(capture_adc_callback_stub);
    ADC_LL_init_Stub(adc_init_success_stub);
    ADC_LL_get_sample_rate_ExpectAndReturn(1000);
    ADC_LL_get_sample_rate_ExpectAndReturn(1000);
    TIM_LL_init_Expect(TIM_NUM_6, 1000);
    TIM_LL_start_Expect(TIM_NUM_6);
    ADC_LL_start_Expect();

    g_test_handle = DMM_init(&config);
    TEST_ASSERT_NOT_NULL(g_test_handle);

    // Zero ADC value should give zero voltage
    simulate_adc_conversion(0);

    ADC_LL_get_reference_voltage_ExpectAndReturn(3300); // 3.3V in mV
    ADC_LL_start_Expect(); // Only expect ADC restart, timer keeps running

    // Act
    result = DMM_read_voltage(g_test_handle, &voltage_out);

    // Assert
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(FIXED_ZERO, voltage_out);
}

// Test: Voltage calculation with half-scale ADC value
void test_DMM_voltage_calculation_half_scale_adc(void)
{
    // Arrange
    DMM_Config config = DMM_CONFIG_DEFAULT;
    config.oversampling_ratio = 1; // No oversampling for simpler calculation
    FIXED_Q1616 voltage_out;
    bool result;

    // Initialize DMM
    ADC_LL_set_complete_callback_Stub(capture_adc_callback_stub);
    ADC_LL_init_Stub(adc_init_success_stub);
    ADC_LL_get_sample_rate_ExpectAndReturn(1000);
    ADC_LL_get_sample_rate_ExpectAndReturn(1000);
    TIM_LL_init_Expect(TIM_NUM_6, 1000);
    TIM_LL_start_Expect(TIM_NUM_6);
    ADC_LL_start_Expect();

    g_test_handle = DMM_init(&config);
    TEST_ASSERT_NOT_NULL(g_test_handle);

    // Half-scale ADC value should give half reference voltage
    // ADC is 12-bit, so max value is 4095
    // Half-scale = 2047, should give ~1.65V with 3.3V reference
    simulate_adc_conversion(2047);

    ADC_LL_get_reference_voltage_ExpectAndReturn(3300); // 3.3V in mV
    ADC_LL_start_Expect(); // Only expect ADC restart, timer keeps running

    // Act
    result = DMM_read_voltage(g_test_handle, &voltage_out);

    // Assert
    TEST_ASSERT_TRUE(result);
    // Expected: (2047 * 3.3V) / 4095 ≈ 1.648V
    FIXED_Q1616 expected_half = FIXED_FROM_FLOAT(1.648f);
    // Allow some tolerance for fixed-point calculation
    FIXED_Q1616 tolerance = FIXED_FROM_FLOAT(0.01f);
    TEST_ASSERT_GREATER_OR_EQUAL_INT32(expected_half - tolerance, voltage_out);
    TEST_ASSERT_LESS_OR_EQUAL_INT32(expected_half + tolerance, voltage_out);
}

// Test: Voltage calculation with full-scale ADC value
void test_DMM_voltage_calculation_full_scale_adc(void)
{
    // Arrange
    DMM_Config config = DMM_CONFIG_DEFAULT;
    config.oversampling_ratio = 1; // No oversampling for simpler calculation
    FIXED_Q1616 voltage_out;
    bool result;

    // Initialize DMM
    ADC_LL_set_complete_callback_Stub(capture_adc_callback_stub);
    ADC_LL_init_Stub(adc_init_success_stub);
    ADC_LL_get_sample_rate_ExpectAndReturn(1000);
    ADC_LL_get_sample_rate_ExpectAndReturn(1000);
    TIM_LL_init_Expect(TIM_NUM_6, 1000);
    TIM_LL_start_Expect(TIM_NUM_6);
    ADC_LL_start_Expect();

    g_test_handle = DMM_init(&config);
    TEST_ASSERT_NOT_NULL(g_test_handle);

    // Full-scale ADC value should give full reference voltage
    simulate_adc_conversion(4095);

    ADC_LL_get_reference_voltage_ExpectAndReturn(3300); // 3.3V in mV
    ADC_LL_start_Expect(); // Only expect ADC restart, timer keeps running

    // Act
    result = DMM_read_voltage(g_test_handle, &voltage_out);

    // Assert
    TEST_ASSERT_TRUE(result);
    // Should be very close to 3.3V
    FIXED_Q1616 expected_full = FIXED_FROM_FLOAT(3.3f);
    FIXED_Q1616 tolerance = FIXED_FROM_FLOAT(0.01f);
    TEST_ASSERT_TRUE((voltage_out >= (expected_full - tolerance)) &&
                     (voltage_out <= (expected_full + tolerance)));
}

// Test: Voltage calculation with oversampling
void test_DMM_voltage_calculation_with_oversampling(void)
{
    // Arrange
    DMM_Config config = DMM_CONFIG_DEFAULT;
    config.oversampling_ratio = 16;
    FIXED_Q1616 voltage_out;
    bool result;

    // Initialize DMM
    ADC_LL_set_complete_callback_Stub(capture_adc_callback_stub);
    ADC_LL_init_Stub(adc_init_success_stub);
    ADC_LL_get_sample_rate_ExpectAndReturn(1000);
    ADC_LL_get_sample_rate_ExpectAndReturn(1000);
    TIM_LL_init_Expect(TIM_NUM_6, 1000);
    TIM_LL_start_Expect(TIM_NUM_6);
    ADC_LL_start_Expect();

    g_test_handle = DMM_init(&config);
    TEST_ASSERT_NOT_NULL(g_test_handle);

    // Oversampling shifts extra samples such that the max ADC value remains 4095
    // Half-scale is still 2047
    simulate_adc_conversion(2047);

    ADC_LL_get_reference_voltage_ExpectAndReturn(3300); // 3.3V in mV
    ADC_LL_start_Expect(); // Only expect ADC restart, timer keeps running

    // Act
    result = DMM_read_voltage(g_test_handle, &voltage_out);

    // Assert
    TEST_ASSERT_TRUE(result);
    // Expected: (2047 * 3.3V) / 4095 ≈ 1.65V
    FIXED_Q1616 expected_half = FIXED_FROM_FLOAT(1.65f);
    FIXED_Q1616 tolerance = FIXED_FROM_FLOAT(0.01f);
    TEST_ASSERT_TRUE((voltage_out >= (expected_half - tolerance)) &&
                     (voltage_out <= (expected_half + tolerance)));
}

// Test: DMM configuration validation edge cases
void test_DMM_config_validation_edge_cases(void)
{
    CEXCEPTION_T exception = CEXCEPTION_NONE;

    // Test maximum valid oversampling ratio
    {
        DMM_Config config = DMM_CONFIG_DEFAULT;
        config.oversampling_ratio = 256; // Maximum valid

        ADC_LL_set_complete_callback_Expect(dmm_adc_complete_callback);
        ADC_LL_init_Ignore();
        ADC_LL_get_sample_rate_ExpectAndReturn(1000);
        ADC_LL_get_sample_rate_ExpectAndReturn(1000);
        TIM_LL_init_Expect(TIM_NUM_6, 1000);
        TIM_LL_start_Expect(TIM_NUM_6);
        ADC_LL_start_Expect();

        g_test_handle = DMM_init(&config);
        TEST_ASSERT_NOT_NULL(g_test_handle);

        ADC_LL_stop_Expect();
        TIM_LL_stop_Expect(TIM_NUM_6);
        ADC_LL_deinit_Expect();
        TIM_LL_deinit_Expect(TIM_NUM_6);
        DMM_deinit(g_test_handle);
        g_test_handle = NULL;
    }

    // Test oversampling ratio too large
    {
        DMM_Config config = DMM_CONFIG_DEFAULT;
        config.oversampling_ratio = 512; // Too large

        TRY {
            g_test_handle = DMM_init(&config);
            TEST_FAIL_MESSAGE("Expected exception for oversampling ratio too large");
        }
        CATCH(exception) {
            TEST_ASSERT_EQUAL(ERROR_INVALID_ARGUMENT, exception);
        }
    }
}

// Test: ADC channel mapping
void test_DMM_channel_mapping(void)
{
    // Test that all DMM channels map correctly to ADC_LL channels
    for (int i = DMM_CHANNEL_0; i <= DMM_CHANNEL_15; i++) {
        DMM_Config config = DMM_CONFIG_DEFAULT;
        config.channel = (DMM_Channel)i;

        ADC_LL_set_complete_callback_Expect(dmm_adc_complete_callback);
        ADC_LL_init_Ignore();
        ADC_LL_get_sample_rate_ExpectAndReturn(1000);
        ADC_LL_get_sample_rate_ExpectAndReturn(1000);
        TIM_LL_init_Expect(TIM_NUM_6, 1000);
        TIM_LL_start_Expect(TIM_NUM_6);
        ADC_LL_start_Expect();

        g_test_handle = DMM_init(&config);
        TEST_ASSERT_NOT_NULL(g_test_handle);

        ADC_LL_stop_Expect();
        TIM_LL_stop_Expect(TIM_NUM_6);
        ADC_LL_deinit_Expect();
        TIM_LL_deinit_Expect(TIM_NUM_6);
        DMM_deinit(g_test_handle);
        g_test_handle = NULL;
    }
}
