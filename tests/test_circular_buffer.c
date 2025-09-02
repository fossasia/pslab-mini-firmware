/**
 * @file test_circular_buffer.c
 * @brief Unit tests for circular buffer implementation
 *
 * This file contains comprehensive unit tests for the circular buffer
 * implementation used throughout the PSLab firmware.
 */

#include <stdlib.h>
#include <string.h>

#include "unity.h"

#include "util/error.h"
#include "util/util.h"

// Test fixtures
static CircularBuffer g_test_buffer;
static uint8_t g_test_data[16];  // Small buffer for testing
static uint8_t g_large_data[256]; // Larger buffer for extensive tests

void setUp(void)
{
    // Initialize test buffer before each test
    circular_buffer_init(&g_test_buffer, g_test_data, sizeof(g_test_data));
}

void tearDown(void)
{
    // Clean up after each test
    circular_buffer_reset(&g_test_buffer);
}

// Test initialization
void test_circular_buffer_init(void)
{
    CircularBuffer cb;
    uint8_t buffer[32];

    circular_buffer_init(&cb, buffer, sizeof(buffer));

    TEST_ASSERT_EQUAL_PTR(buffer, cb.buffer);
    TEST_ASSERT_EQUAL_UINT32(0, cb.head);
    TEST_ASSERT_EQUAL_UINT32(0, cb.tail);
    TEST_ASSERT_EQUAL_UINT32(sizeof(buffer), cb.size);
}

// Test empty buffer state
void test_circular_buffer_is_empty_when_initialized(void)
{
    TEST_ASSERT_TRUE(circular_buffer_is_empty(&g_test_buffer));
    TEST_ASSERT_FALSE(circular_buffer_is_full(&g_test_buffer));
    TEST_ASSERT_EQUAL_UINT32(0, circular_buffer_available(&g_test_buffer));
    TEST_ASSERT_EQUAL_UINT32(15, circular_buffer_free_space(&g_test_buffer)); // size - 1
}

// Test that non-power-of-two size results in unusable buffer
void test_circular_buffer_init_non_power_of_two_size(void)
{
    Error exc;
    CircularBuffer cb;
    uint8_t buffer[30]; // Not a power of two

    TRY
    {
        circular_buffer_init(&cb, buffer, sizeof(buffer));
    }
    CATCH(exc)
    {
        // Expected error for non-power-of-two size
        TEST_ASSERT_EQUAL(ERROR_INVALID_ARGUMENT, exc);
        return;
    }
}

// Test single byte operations
void test_circular_buffer_put_get_single_byte(void)
{
    uint8_t test_byte = 0xAB;
    uint8_t read_byte;

    // Put a byte
    TEST_ASSERT_TRUE(circular_buffer_put(&g_test_buffer, test_byte));
    TEST_ASSERT_FALSE(circular_buffer_is_empty(&g_test_buffer));
    TEST_ASSERT_FALSE(circular_buffer_is_full(&g_test_buffer));
    TEST_ASSERT_EQUAL_UINT32(1, circular_buffer_available(&g_test_buffer));
    TEST_ASSERT_EQUAL_UINT32(14, circular_buffer_free_space(&g_test_buffer));

    // Get the byte back
    TEST_ASSERT_TRUE(circular_buffer_get(&g_test_buffer, &read_byte));
    TEST_ASSERT_EQUAL_UINT8(test_byte, read_byte);
    TEST_ASSERT_TRUE(circular_buffer_is_empty(&g_test_buffer));
    TEST_ASSERT_EQUAL_UINT32(0, circular_buffer_available(&g_test_buffer));
    TEST_ASSERT_EQUAL_UINT32(15, circular_buffer_free_space(&g_test_buffer));
}

// Test getting from empty buffer
void test_circular_buffer_get_from_empty_buffer(void)
{
    uint8_t read_byte;

    TEST_ASSERT_FALSE(circular_buffer_get(&g_test_buffer, &read_byte));
    TEST_ASSERT_TRUE(circular_buffer_is_empty(&g_test_buffer));
}

// Test filling buffer to capacity
void test_circular_buffer_fill_to_capacity(void)
{
    uint8_t i;

    // Fill buffer to capacity (size - 1 elements due to circular buffer design)
    for (i = 0; i < 15; i++) {
        TEST_ASSERT_TRUE(circular_buffer_put(&g_test_buffer, i));
        TEST_ASSERT_EQUAL_UINT32(i + 1, circular_buffer_available(&g_test_buffer));
        TEST_ASSERT_EQUAL_UINT32(15 - i - 1, circular_buffer_free_space(&g_test_buffer));
    }

    TEST_ASSERT_TRUE(circular_buffer_is_full(&g_test_buffer));
    TEST_ASSERT_FALSE(circular_buffer_is_empty(&g_test_buffer));

    // Try to add one more (should fail)
    TEST_ASSERT_FALSE(circular_buffer_put(&g_test_buffer, 99));
    TEST_ASSERT_TRUE(circular_buffer_is_full(&g_test_buffer));
}

// Test putting to full buffer
void test_circular_buffer_put_to_full_buffer(void)
{
    uint8_t i;

    // Fill buffer
    for (i = 0; i < 15; i++) {
        circular_buffer_put(&g_test_buffer, i);
    }

    // Try to add to full buffer
    TEST_ASSERT_FALSE(circular_buffer_put(&g_test_buffer, 100));
    TEST_ASSERT_TRUE(circular_buffer_is_full(&g_test_buffer));
}

// Test wrap-around behavior
void test_circular_buffer_wrap_around(void)
{
    uint8_t i, read_byte;

    // Fill buffer partially
    for (i = 0; i < 8; i++) {
        circular_buffer_put(&g_test_buffer, i);
    }

    // Read some bytes
    for (i = 0; i < 4; i++) {
        TEST_ASSERT_TRUE(circular_buffer_get(&g_test_buffer, &read_byte));
        TEST_ASSERT_EQUAL_UINT8(i, read_byte);
    }

    // Add more bytes (should wrap around)
    for (i = 8; i < 15; i++) {
        TEST_ASSERT_TRUE(circular_buffer_put(&g_test_buffer, i));
    }

    // Verify all remaining bytes can be read in order
    for (i = 4; i < 15; i++) {
        TEST_ASSERT_TRUE(circular_buffer_get(&g_test_buffer, &read_byte));
        TEST_ASSERT_EQUAL_UINT8(i, read_byte);
    }

    TEST_ASSERT_TRUE(circular_buffer_is_empty(&g_test_buffer));
}

// Test reset functionality
void test_circular_buffer_reset(void)
{
    uint8_t i;

    // Add some data
    for (i = 0; i < 5; i++) {
        circular_buffer_put(&g_test_buffer, i);
    }

    TEST_ASSERT_FALSE(circular_buffer_is_empty(&g_test_buffer));
    TEST_ASSERT_EQUAL_UINT32(5, circular_buffer_available(&g_test_buffer));

    // Reset buffer
    circular_buffer_reset(&g_test_buffer);

    TEST_ASSERT_TRUE(circular_buffer_is_empty(&g_test_buffer));
    TEST_ASSERT_FALSE(circular_buffer_is_full(&g_test_buffer));
    TEST_ASSERT_EQUAL_UINT32(0, circular_buffer_available(&g_test_buffer));
    TEST_ASSERT_EQUAL_UINT32(15, circular_buffer_free_space(&g_test_buffer));
}

// Test multi-byte write operation
void test_circular_buffer_write_multiple_bytes(void)
{
    uint8_t write_data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    uint8_t read_byte;
    uint32_t bytes_written;
    uint8_t i;

    bytes_written = circular_buffer_write(&g_test_buffer, write_data, sizeof(write_data));

    TEST_ASSERT_EQUAL_UINT32(sizeof(write_data), bytes_written);
    TEST_ASSERT_EQUAL_UINT32(sizeof(write_data), circular_buffer_available(&g_test_buffer));

    // Verify data was written correctly
    for (i = 0; i < sizeof(write_data); i++) {
        TEST_ASSERT_TRUE(circular_buffer_get(&g_test_buffer, &read_byte));
        TEST_ASSERT_EQUAL_UINT8(write_data[i], read_byte);
    }
}

// Test multi-byte write with insufficient space
void test_circular_buffer_write_insufficient_space(void)
{
    uint8_t write_data[20]; // Larger than buffer capacity
    uint32_t bytes_written;
    uint8_t i;

    // Initialize write data
    for (i = 0; i < sizeof(write_data); i++) {
        write_data[i] = i;
    }

    bytes_written = circular_buffer_write(&g_test_buffer, write_data, sizeof(write_data));

    // Should only write as much as fits (15 bytes max)
    TEST_ASSERT_EQUAL_UINT32(15, bytes_written);
    TEST_ASSERT_TRUE(circular_buffer_is_full(&g_test_buffer));
}

// Test multi-byte write which fits exactly
void test_circular_buffer_write_exact_fit(void)
{
    uint8_t write_data[15]; // Exactly buffer size - 1
    uint32_t bytes_written;
    uint8_t i;

    // Initialize write data
    for (i = 0; i < sizeof(write_data); i++) {
        write_data[i] = i;
    }

    bytes_written = circular_buffer_write(&g_test_buffer, write_data, sizeof(write_data));

    // Should write exactly 15 bytes
    TEST_ASSERT_EQUAL_UINT32(15, bytes_written);
    TEST_ASSERT_TRUE(circular_buffer_is_full(&g_test_buffer));
}

// Test multi-byte read operation
void test_circular_buffer_read_multiple_bytes(void)
{
    uint8_t write_data[] = {0x10, 0x20, 0x30, 0x40, 0x50, 0x60};
    uint8_t read_data[10];
    uint32_t bytes_read;
    uint8_t i;

    // Write data first
    circular_buffer_write(&g_test_buffer, write_data, sizeof(write_data));

    // Read data back
    bytes_read = circular_buffer_read(&g_test_buffer, read_data, sizeof(read_data));

    TEST_ASSERT_EQUAL_UINT32(sizeof(write_data), bytes_read);

    // Verify data integrity
    for (i = 0; i < sizeof(write_data); i++) {
        TEST_ASSERT_EQUAL_UINT8(write_data[i], read_data[i]);
    }

    TEST_ASSERT_TRUE(circular_buffer_is_empty(&g_test_buffer));
}

// Test read from empty buffer
void test_circular_buffer_read_from_empty(void)
{
    uint8_t read_data[10];
    uint32_t bytes_read;

    bytes_read = circular_buffer_read(&g_test_buffer, read_data, sizeof(read_data));

    TEST_ASSERT_EQUAL_UINT32(0, bytes_read);
    TEST_ASSERT_TRUE(circular_buffer_is_empty(&g_test_buffer));
}

// Test partial read
void test_circular_buffer_partial_read(void)
{
    uint8_t write_data[] = {0xAA, 0xBB, 0xCC};
    uint8_t read_data[10];
    uint32_t bytes_read;

    // Write 3 bytes
    circular_buffer_write(&g_test_buffer, write_data, sizeof(write_data));

    // Try to read 10 bytes (should only read 3)
    bytes_read = circular_buffer_read(&g_test_buffer, read_data, sizeof(read_data));

    TEST_ASSERT_EQUAL_UINT32(3, bytes_read);
    TEST_ASSERT_EQUAL_UINT8(0xAA, read_data[0]);
    TEST_ASSERT_EQUAL_UINT8(0xBB, read_data[1]);
    TEST_ASSERT_EQUAL_UINT8(0xCC, read_data[2]);
    TEST_ASSERT_TRUE(circular_buffer_is_empty(&g_test_buffer));
}

// Test free space calculation during operations
void test_circular_buffer_free_space_calculation(void)
{
    uint8_t i;

    // Initially should have size-1 free space
    TEST_ASSERT_EQUAL_UINT32(15, circular_buffer_free_space(&g_test_buffer));

    // Add bytes and verify free space decreases
    for (i = 0; i < 10; i++) {
        circular_buffer_put(&g_test_buffer, i);
        TEST_ASSERT_EQUAL_UINT32(15 - i - 1, circular_buffer_free_space(&g_test_buffer));
    }

    // Remove some bytes and verify free space increases
    uint8_t dummy;
    for (i = 0; i < 5; i++) {
        circular_buffer_get(&g_test_buffer, &dummy);
        TEST_ASSERT_EQUAL_UINT32(5 + i + 1, circular_buffer_free_space(&g_test_buffer));
    }
}

// Test with larger buffer for wrap-around scenarios
void test_circular_buffer_large_buffer_operations(void)
{
    CircularBuffer large_cb;
    uint8_t write_pattern[100];
    uint8_t read_pattern[100];
    uint32_t i;

    circular_buffer_init(&large_cb, g_large_data, sizeof(g_large_data));

    // Create test pattern
    for (i = 0; i < sizeof(write_pattern); i++) {
        write_pattern[i] = (uint8_t)(i & 0xFF);
    }

    // Write and read multiple times to test wrap-around
    for (i = 0; i < 3; i++) {
        uint32_t bytes_written = circular_buffer_write(&large_cb, write_pattern, sizeof(write_pattern));
        TEST_ASSERT_EQUAL_UINT32(sizeof(write_pattern), bytes_written);

        uint32_t bytes_read = circular_buffer_read(&large_cb, read_pattern, sizeof(read_pattern));
        TEST_ASSERT_EQUAL_UINT32(sizeof(write_pattern), bytes_read);

        // Verify data integrity
        TEST_ASSERT_EQUAL_UINT8_ARRAY(write_pattern, read_pattern, sizeof(write_pattern));
        TEST_ASSERT_TRUE(circular_buffer_is_empty(&large_cb));
    }
}

// Test edge case: zero-length operations
void test_circular_buffer_zero_length_operations(void)
{
    uint8_t dummy_data[10];
    uint32_t bytes_written, bytes_read;

    // Zero-length write
    bytes_written = circular_buffer_write(&g_test_buffer, dummy_data, 0);
    TEST_ASSERT_EQUAL_UINT32(0, bytes_written);
    TEST_ASSERT_TRUE(circular_buffer_is_empty(&g_test_buffer));

    // Zero-length read
    bytes_read = circular_buffer_read(&g_test_buffer, dummy_data, 0);
    TEST_ASSERT_EQUAL_UINT32(0, bytes_read);
    TEST_ASSERT_TRUE(circular_buffer_is_empty(&g_test_buffer));
}

// Test boundary conditions
void test_circular_buffer_boundary_conditions(void)
{
    uint8_t test_data[15]; // Exactly buffer size - 1
    uint8_t read_data[15];
    uint32_t i;

    // Initialize test data
    for (i = 0; i < sizeof(test_data); i++) {
        test_data[i] = (uint8_t)(i + 1);
    }

    // Fill buffer to exact capacity
    uint32_t bytes_written = circular_buffer_write(&g_test_buffer, test_data, sizeof(test_data));
    TEST_ASSERT_EQUAL_UINT32(15, bytes_written);
    TEST_ASSERT_TRUE(circular_buffer_is_full(&g_test_buffer));
    TEST_ASSERT_EQUAL_UINT32(0, circular_buffer_free_space(&g_test_buffer));

    // Read everything back
    uint32_t bytes_read = circular_buffer_read(&g_test_buffer, read_data, sizeof(read_data));
    TEST_ASSERT_EQUAL_UINT32(15, bytes_read);
    TEST_ASSERT_TRUE(circular_buffer_is_empty(&g_test_buffer));

    // Verify data integrity
    TEST_ASSERT_EQUAL_UINT8_ARRAY(test_data, read_data, sizeof(test_data));
}
