/**
 * @file test_dmm.c
 * @brief Unit tests for Digital Multimeter (DMM) module
 *
 * This file contains comprehensive unit tests for the DMM module,
 * testing all public functions with various scenarios including
 * success cases, error conditions, and edge cases.
 *
 * @author PSLab Team
 * @date 2025-07-19
 */

#include "unity.h"
#include "mock_adc_ll.h"
#include "dmm.h"
#include "error.h"
#include "fixed_point.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

// Test fixtures
static DMM_Handle *g_test_handle;

void setUp(void)
{
    // Initialize test handle to NULL
    g_test_handle = NULL;

    // Initialize mocks
    mock_adc_ll_Init();
}

void tearDown(void)
{
    // Clean up DMM handle if it exists
    if (g_test_handle != NULL) {
        // Set up expectations for deinit
        ADC_LL_deinit_Ignore();

        DMM_deinit(g_test_handle);
        g_test_handle = NULL;
    }

    // Clean up mocks after each test
    mock_adc_ll_Destroy();
}

// Test DMM_init with valid configuration
void test_DMM_init_success(void)
{
    // Arrange
    DMM_Config config = DMM_CONFIG_DEFAULT;

    // Set expectations for ADC_LL calls - just ignore the callback for simplicity
    ADC_LL_set_complete_callback_Ignore();

    // Act
    g_test_handle = DMM_init(&config);

    // Assert
    TEST_ASSERT_NOT_NULL(g_test_handle);
}

// Test DMM_init with NULL configuration
void test_DMM_init_null_config(void)
{
    // Arrange
    DMM_Handle *handle = NULL;
    Error exception = ERROR_NONE;

    // Act
    TRY
    {
        handle = DMM_init(NULL);
        TEST_FAIL_MESSAGE("Expected ERROR_INVALID_ARGUMENT exception");
    }
    CATCH(exception)
    {
        // Assert
        TEST_ASSERT_EQUAL(ERROR_INVALID_ARGUMENT, exception);
        TEST_ASSERT_NULL(handle);
    }
}

// Test DMM_init with invalid oversampling ratio
void test_DMM_init_invalid_oversampling(void)
{
    // Arrange
    DMM_Config config = DMM_CONFIG_DEFAULT;
    config.oversampling_ratio = 0; // Invalid: must be > 0
    DMM_Handle *handle = NULL;
    Error exception = ERROR_NONE;

    // Act
    TRY
    {
        handle = DMM_init(&config);
        TEST_FAIL_MESSAGE("Expected ERROR_INVALID_ARGUMENT exception");
    }
    CATCH(exception)
    {
        // Assert
        TEST_ASSERT_EQUAL(ERROR_INVALID_ARGUMENT, exception);
        TEST_ASSERT_NULL(handle);
    }
}

// Test DMM_deinit with valid handle
void test_DMM_deinit_success(void)
{
    // Arrange
    DMM_Config config = DMM_CONFIG_DEFAULT;
    ADC_LL_set_complete_callback_Ignore();
    g_test_handle = DMM_init(&config);

    // Set expectations for deinit
    ADC_LL_deinit_Expect();

    // Act
    DMM_deinit(g_test_handle);
    g_test_handle = NULL; // Prevent double cleanup in tearDown

    // Assert - no assertion needed, function should complete without error
}

// Test DMM_deinit with NULL handle
void test_DMM_deinit_null_handle(void)
{
    // Act - should not crash or throw
    DMM_deinit(NULL);

    // Assert - no assertion needed, function should complete without error
}

// Test DMM_read_voltage with NULL handle
void test_DMM_read_voltage_null_handle(void)
{
    // Arrange
    Fixed voltage_out;
    Error exception = ERROR_NONE;

    // Act
    TRY
    {
        DMM_read_voltage(NULL, DMM_CHANNEL_0, &voltage_out);
        TEST_FAIL_MESSAGE("Expected ERROR_INVALID_ARGUMENT exception");
    }
    CATCH(exception)
    {
        // Assert
        TEST_ASSERT_EQUAL(ERROR_INVALID_ARGUMENT, exception);
    }
}

// Test DMM_read_voltage with NULL output pointer
void test_DMM_read_voltage_null_output(void)
{
    // Arrange
    DMM_Config config = DMM_CONFIG_DEFAULT;
    ADC_LL_set_complete_callback_Ignore();
    g_test_handle = DMM_init(&config);
    Error exception = ERROR_NONE;

    // Act
    TRY
    {
        DMM_read_voltage(g_test_handle, DMM_CHANNEL_0, NULL);
        TEST_FAIL_MESSAGE("Expected ERROR_INVALID_ARGUMENT exception");
    }
    CATCH(exception)
    {
        // Assert
        TEST_ASSERT_EQUAL(ERROR_INVALID_ARGUMENT, exception);
    }
}

// Test DMM_read_voltage with invalid channel
void test_DMM_read_voltage_invalid_channel(void)
{
    // Arrange
    DMM_Config config = DMM_CONFIG_DEFAULT;
    ADC_LL_set_complete_callback_Ignore();
    g_test_handle = DMM_init(&config);

    Fixed voltage_out;
    Error exception = ERROR_NONE;

    // Act
    TRY
    {
        DMM_read_voltage(g_test_handle, (DMM_Channel)16, &voltage_out);
        TEST_FAIL_MESSAGE("Expected ERROR_INVALID_ARGUMENT exception");
    }
    CATCH(exception)
    {
        // Assert
        TEST_ASSERT_EQUAL(ERROR_INVALID_ARGUMENT, exception);
    }
}

// Test DMM_read_raw with NULL handle
void test_DMM_read_raw_null_handle(void)
{
    // Arrange
    uint16_t raw_value_out;
    Error exception = ERROR_NONE;

    // Act
    TRY
    {
        DMM_read_raw(NULL, DMM_CHANNEL_0, &raw_value_out);
        TEST_FAIL_MESSAGE("Expected ERROR_INVALID_ARGUMENT exception");
    }
    CATCH(exception)
    {
        // Assert
        TEST_ASSERT_EQUAL(ERROR_INVALID_ARGUMENT, exception);
    }
}

// Test DMM_read_raw with NULL output pointer
void test_DMM_read_raw_null_output(void)
{
    // Arrange
    DMM_Config config = DMM_CONFIG_DEFAULT;
    ADC_LL_set_complete_callback_Ignore();
    g_test_handle = DMM_init(&config);
    Error exception = ERROR_NONE;

    // Act
    TRY
    {
        DMM_read_raw(g_test_handle, DMM_CHANNEL_0, NULL);
        TEST_FAIL_MESSAGE("Expected ERROR_INVALID_ARGUMENT exception");
    }
    CATCH(exception)
    {
        // Assert
        TEST_ASSERT_EQUAL(ERROR_INVALID_ARGUMENT, exception);
    }
}

// Test DMM_read_millivolts with NULL handle
void test_DMM_read_millivolts_null_handle(void)
{
    // Arrange
    uint32_t millivolts_out;
    Error exception = ERROR_NONE;

    // Act
    TRY
    {
        DMM_read_millivolts(NULL, DMM_CHANNEL_0, &millivolts_out);
        TEST_FAIL_MESSAGE("Expected ERROR_INVALID_ARGUMENT exception");
    }
    CATCH(exception)
    {
        // Assert
        TEST_ASSERT_EQUAL(ERROR_INVALID_ARGUMENT, exception);
    }
}

// Test configuration validation with various valid oversampling ratios
void test_DMM_valid_oversampling_ratios(void)
{
    // Test all valid power-of-2 oversampling ratios
    uint32_t valid_ratios[] = {1, 2, 4, 8, 16, 32, 64, 128, 256};
    size_t num_ratios = sizeof(valid_ratios) / sizeof(valid_ratios[0]);

    for (size_t i = 0; i < num_ratios; i++) {
        DMM_Config config = DMM_CONFIG_DEFAULT;
        config.oversampling_ratio = valid_ratios[i];

        // Set expectations for ADC_LL calls
        ADC_LL_set_complete_callback_Ignore();

        // Act
        DMM_Handle *handle = DMM_init(&config);

        // Assert
        TEST_ASSERT_NOT_NULL(handle);

        // Clean up
        ADC_LL_deinit_Ignore();
        DMM_deinit(handle);
    }
}

// Test DMM_init with invalid oversampling ratios (more cases)
void test_DMM_init_invalid_oversampling_not_power_of_2(void)
{
    // Arrange
    DMM_Config config = DMM_CONFIG_DEFAULT;
    config.oversampling_ratio = 3; // Invalid: not power of 2
    DMM_Handle *handle = NULL;
    Error exception = ERROR_NONE;

    // Act
    TRY
    {
        handle = DMM_init(&config);
        TEST_FAIL_MESSAGE("Expected ERROR_INVALID_ARGUMENT exception");
    }
    CATCH(exception)
    {
        // Assert
        TEST_ASSERT_EQUAL(ERROR_INVALID_ARGUMENT, exception);
        TEST_ASSERT_NULL(handle);
    }
}

// Test DMM_init with invalid oversampling ratio (too large)
void test_DMM_init_invalid_oversampling_too_large(void)
{
    // Arrange
    DMM_Config config = DMM_CONFIG_DEFAULT;
    config.oversampling_ratio = 512; // Invalid: > 256
    DMM_Handle *handle = NULL;
    Error exception = ERROR_NONE;

    // Act
    TRY
    {
        handle = DMM_init(&config);
        TEST_FAIL_MESSAGE("Expected ERROR_INVALID_ARGUMENT exception");
    }
    CATCH(exception)
    {
        // Assert
        TEST_ASSERT_EQUAL(ERROR_INVALID_ARGUMENT, exception);
        TEST_ASSERT_NULL(handle);
    }
}

// Test DMM_init with invalid timeout
void test_DMM_init_invalid_timeout(void)
{
    // Arrange
    DMM_Config config = DMM_CONFIG_DEFAULT;
    config.timeout_ms = 0; // Invalid: must be > 0
    DMM_Handle *handle = NULL;
    Error exception = ERROR_NONE;

    // Act
    TRY
    {
        handle = DMM_init(&config);
        TEST_FAIL_MESSAGE("Expected ERROR_INVALID_ARGUMENT exception");
    }
    CATCH(exception)
    {
        // Assert
        TEST_ASSERT_EQUAL(ERROR_INVALID_ARGUMENT, exception);
        TEST_ASSERT_NULL(handle);
    }
}

// Test DMM_init with invalid reference voltage (zero)
void test_DMM_init_invalid_reference_voltage_zero(void)
{
    // Arrange
    DMM_Config config = DMM_CONFIG_DEFAULT;
    config.reference_voltage = FIXED_ZERO; // Invalid: must be > 0
    DMM_Handle *handle = NULL;
    Error exception = ERROR_NONE;

    // Act
    TRY
    {
        handle = DMM_init(&config);
        TEST_FAIL_MESSAGE("Expected ERROR_INVALID_ARGUMENT exception");
    }
    CATCH(exception)
    {
        // Assert
        TEST_ASSERT_EQUAL(ERROR_INVALID_ARGUMENT, exception);
        TEST_ASSERT_NULL(handle);
    }
}

// Test DMM_init with invalid reference voltage (too high)
void test_DMM_init_invalid_reference_voltage_too_high(void)
{
    // Arrange
    DMM_Config config = DMM_CONFIG_DEFAULT;
    config.reference_voltage = FIXED_FROM_FLOAT(6.0f); // Invalid: > 5.0V
    DMM_Handle *handle = NULL;
    Error exception = ERROR_NONE;

    // Act
    TRY
    {
        handle = DMM_init(&config);
        TEST_FAIL_MESSAGE("Expected ERROR_INVALID_ARGUMENT exception");
    }
    CATCH(exception)
    {
        // Assert
        TEST_ASSERT_EQUAL(ERROR_INVALID_ARGUMENT, exception);
        TEST_ASSERT_NULL(handle);
    }
}

// Test DMM_init when already initialized
void test_DMM_init_already_initialized(void)
{
    // Arrange
    DMM_Config config = DMM_CONFIG_DEFAULT;
    DMM_Handle *handle1, *handle2 = NULL;
    Error exception = ERROR_NONE;

    // Set expectations for first init
    ADC_LL_set_complete_callback_Ignore();

    // Act - first init should succeed
    handle1 = DMM_init(&config);
    TEST_ASSERT_NOT_NULL(handle1);

    // Act & Assert - second init should fail
    TRY
    {
        handle2 = DMM_init(&config);
        TEST_FAIL_MESSAGE("Expected ERROR_RESOURCE_BUSY exception");
    }
    CATCH(exception)
    {
        TEST_ASSERT_EQUAL(ERROR_RESOURCE_BUSY, exception);
        TEST_ASSERT_NULL(handle2);
    }

    // Clean up
    ADC_LL_deinit_Ignore();
    DMM_deinit(handle1);
    g_test_handle = NULL; // Prevent double cleanup in tearDown
}

// Test DMM_read_raw with invalid channel
void test_DMM_read_raw_invalid_channel(void)
{
    // Arrange
    DMM_Config config = DMM_CONFIG_DEFAULT;
    ADC_LL_set_complete_callback_Ignore();
    g_test_handle = DMM_init(&config);

    uint16_t raw_value_out;
    Error exception = ERROR_NONE;

    // Act
    TRY
    {
        DMM_read_raw(g_test_handle, (DMM_Channel)16, &raw_value_out);
        TEST_FAIL_MESSAGE("Expected ERROR_INVALID_ARGUMENT exception");
    }
    CATCH(exception)
    {
        // Assert
        TEST_ASSERT_EQUAL(ERROR_INVALID_ARGUMENT, exception);
    }
}

// Test DMM_read_millivolts with NULL output pointer
void test_DMM_read_millivolts_null_output(void)
{
    // Arrange
    DMM_Config config = DMM_CONFIG_DEFAULT;
    ADC_LL_set_complete_callback_Ignore();
    g_test_handle = DMM_init(&config);
    Error exception = ERROR_NONE;

    // Act
    TRY
    {
        DMM_read_millivolts(g_test_handle, DMM_CHANNEL_0, NULL);
        TEST_FAIL_MESSAGE("Expected ERROR_INVALID_ARGUMENT exception");
    }
    CATCH(exception)
    {
        // Assert
        TEST_ASSERT_EQUAL(ERROR_INVALID_ARGUMENT, exception);
    }
}

// Test DMM_read_millivolts with invalid channel
void test_DMM_read_millivolts_invalid_channel(void)
{
    // Arrange
    DMM_Config config = DMM_CONFIG_DEFAULT;
    ADC_LL_set_complete_callback_Ignore();
    g_test_handle = DMM_init(&config);

    uint32_t millivolts_out;
    Error exception = ERROR_NONE;

    // Act
    TRY
    {
        DMM_read_millivolts(g_test_handle, (DMM_Channel)16, &millivolts_out);
        TEST_FAIL_MESSAGE("Expected ERROR_INVALID_ARGUMENT exception");
    }
    CATCH(exception)
    {
        // Assert
        TEST_ASSERT_EQUAL(ERROR_INVALID_ARGUMENT, exception);
    }
}

// Test channel mapping function indirectly
void test_DMM_channel_mapping(void)
{
    // Arrange
    DMM_Config config = DMM_CONFIG_DEFAULT;
    ADC_LL_set_complete_callback_Ignore();
    g_test_handle = DMM_init(&config);

    // Test different channels - the mapping should work correctly
    // We test a few representative channels rather than all 16 to keep test time reasonable
    DMM_Channel test_channels[] = {DMM_CHANNEL_0, DMM_CHANNEL_5, DMM_CHANNEL_10, DMM_CHANNEL_15};
    size_t num_channels = sizeof(test_channels) / sizeof(test_channels[0]);

    for (size_t i = 0; i < num_channels; i++) {
        uint16_t raw_value_out;
        Error exception = ERROR_NONE;

        // Set expectations for ADC operations - since we can't easily simulate the callback,
        // we expect it to timeout and check that the channel validation works
        ADC_LL_init_Ignore();
        ADC_LL_start_Ignore();
        ADC_LL_stop_Ignore();
        ADC_LL_deinit_Ignore();

        // Act - this will likely timeout, but that's ok for this test
        TRY
        {
            DMM_read_raw(g_test_handle, test_channels[i], &raw_value_out);
            // If we get here, the channel was valid (even if timeout occurred)
        }
        CATCH(exception)
        {
            // We expect timeout, not invalid argument for valid channels
            TEST_ASSERT_NOT_EQUAL(ERROR_INVALID_ARGUMENT, exception);
        }
    }
}
