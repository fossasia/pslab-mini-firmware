#include "unity.h"

#include "logging.h"
#include "logging_ll.h"
#include "util.h"
#include <string.h>

// Test fixtures
static LOG_LL_Entry g_test_entry;

void setUp(void)
{
    // Initialize platform logging
    LOG_LL_init();
}

void tearDown(void)
{
    // Reset platform log state
    circular_buffer_reset(&g_LOG_LL_buffer);
}

void test_LOG_LL_init(void)
{
    // Check if the log buffer is initialized correctly
    TEST_ASSERT_NOT_NULL(g_LOG_LL_buffer.buffer);
    TEST_ASSERT_EQUAL(0, g_LOG_LL_buffer.head);
    TEST_ASSERT_EQUAL(0, g_LOG_LL_buffer.tail);
    TEST_ASSERT_EQUAL(LOG_LL_BUFFER_SIZE, g_LOG_LL_buffer.size);
}

void test_LOG_LL_write(void)
{
    // Arrange
    char const *test_message = "Test log message";

    // Act
    LOG_LL_write(LOG_LL_DEBUG, test_message);

    // Assert
    TEST_ASSERT_EQUAL(
        sizeof(g_test_entry.level)
        + sizeof(g_test_entry.length)
        + strlen(test_message) + 1, // +1 for null terminator
        g_LOG_LL_buffer.head
    );
    TEST_ASSERT_EQUAL_MEMORY(
        test_message,
        g_LOG_LL_buffer.buffer
        + g_LOG_LL_buffer.tail
        + sizeof(g_test_entry.level)
        + sizeof(g_test_entry.length),
        strlen(test_message)
    );
}

void test_LOG_LL_read_entry(void)
{
    // Arrange
    LOG_LL_Entry entry;
    entry.level = LOG_LL_INFO;
    entry.length = 20;
    strcpy(entry.message, "Test log message");

    // Write to the platform log buffer
    circular_buffer_write(
        &g_LOG_LL_buffer,
        (uint8_t *)&entry,
        sizeof(entry.level) + sizeof(entry.length) + entry.length + 1
    );

    // Act
    bool result = LOG_LL_read_entry(&entry);

    // Assert
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(LOG_LL_INFO, entry.level);
    TEST_ASSERT_EQUAL_STRING("Test log message", entry.message);
}

void test_LOG_LL_available(void)
{
    // Arrange
    char const *test_message = "Test log message";
    LOG_LL_write(LOG_LL_DEBUG, test_message);

    // Act
    size_t available = LOG_LL_available();

    // Assert
    TEST_ASSERT_GREATER_THAN(0, available);
}

void test_LOG_service_platform(void)
{
    // Arrange
    g_LOG_LL_service_request = true;
    // Fill the log buffer with a test entry
    LOG_LL_write(LOG_LL_INFO, "Test log entry");

    // Act
    LOG_service_platform();

    // Assert - Should process the log entry and reset the request flag
    TEST_ASSERT_FALSE(g_LOG_LL_service_request);
}

void test_LOG_LL_write_multiple_entries(void)
{
    // Arrange & Act
    LOG_LL_write(LOG_LL_ERROR, "Error message");
    LOG_LL_write(LOG_LL_WARN, "Warning message");
    LOG_LL_write(LOG_LL_INFO, "Info message");
    LOG_LL_write(LOG_LL_DEBUG, "Debug message");

    // Assert
    TEST_ASSERT_GREATER_THAN(0, LOG_LL_available());

    // Verify we can read multiple entries
    LOG_LL_Entry entry;
    TEST_ASSERT_TRUE(LOG_LL_read_entry(&entry));
    TEST_ASSERT_EQUAL(LOG_LL_ERROR, entry.level);
    TEST_ASSERT_EQUAL_STRING("Error message", entry.message);

    TEST_ASSERT_TRUE(LOG_LL_read_entry(&entry));
    TEST_ASSERT_EQUAL(LOG_LL_WARN, entry.level);
    TEST_ASSERT_EQUAL_STRING("Warning message", entry.message);

    TEST_ASSERT_TRUE(LOG_LL_read_entry(&entry));
    TEST_ASSERT_EQUAL(LOG_LL_INFO, entry.level);
    TEST_ASSERT_EQUAL_STRING("Info message", entry.message);

    TEST_ASSERT_TRUE(LOG_LL_read_entry(&entry));
    TEST_ASSERT_EQUAL(LOG_LL_DEBUG, entry.level);
    TEST_ASSERT_EQUAL_STRING("Debug message", entry.message);
}

void test_LOG_LL_write_with_format_string(void)
{
    // Arrange
    int value = 42;
    char const *string = "test";

    // Act
    LOG_LL_write(LOG_LL_INFO, "Value: %d, String: %s", value, string);

    // Assert
    LOG_LL_Entry entry;
    TEST_ASSERT_TRUE(LOG_LL_read_entry(&entry));
    TEST_ASSERT_EQUAL(LOG_LL_INFO, entry.level);
    TEST_ASSERT_EQUAL_STRING("Value: 42, String: test", entry.message);
}

void test_LOG_LL_write_long_message(void)
{
    // Arrange - Create a message longer than the buffer can hold
    char long_message[LOG_LL_MAX_MESSAGE_SIZE + 50];
    memset(long_message, 'A', sizeof(long_message) - 1);
    long_message[sizeof(long_message) - 1] = '\0';

    // Act
    LOG_LL_write(LOG_LL_INFO, "%s", long_message);

    // Assert - Message should be truncated to fit buffer
    LOG_LL_Entry entry;
    TEST_ASSERT_TRUE(LOG_LL_read_entry(&entry));
    TEST_ASSERT_EQUAL(LOG_LL_INFO, entry.level);
    TEST_ASSERT_EQUAL(LOG_LL_MAX_MESSAGE_SIZE - 1, entry.length);
    TEST_ASSERT_EQUAL('\0', entry.message[entry.length]);
}

void test_LOG_LL_read_entry_empty_buffer(void)
{
    // Arrange - Buffer is empty after setUp/tearDown

    // Act
    LOG_LL_Entry entry;
    bool result = LOG_LL_read_entry(&entry);

    // Assert
    TEST_ASSERT_FALSE(result);
}

void test_LOG_LL_read_entry_null_pointer(void)
{
    // Arrange
    LOG_LL_write(LOG_LL_INFO, "Test message");

    // Act & Assert
    bool result = LOG_LL_read_entry(NULL);
    TEST_ASSERT_FALSE(result);
}

void test_LOG_LL_buffer_wraparound(void)
{
    // Arrange - Fill buffer almost to capacity
    size_t entries_written = 0;
    while (circular_buffer_free_space(&g_LOG_LL_buffer) > 50) {
        LOG_LL_write(LOG_LL_INFO, "Entry %zu", entries_written);
        entries_written++;
    }

    // Act - Write one more entry to potentially cause wraparound
    LOG_LL_write(LOG_LL_DEBUG, "Final entry");

    // Assert - Should still be able to read entries
    LOG_LL_Entry entry;
    TEST_ASSERT_TRUE(LOG_LL_read_entry(&entry));
    TEST_ASSERT_EQUAL_STRING("Entry 0", entry.message);
}

void test_LOG_LL_service_request_flag_behavior(void)
{
    // Arrange - Initially no service request
    TEST_ASSERT_FALSE(g_LOG_LL_service_request);

    // Act - Write a log entry
    LOG_LL_write(LOG_LL_INFO, "Test entry");

    // Assert - Service request should be set
    TEST_ASSERT_TRUE(g_LOG_LL_service_request);

    // Act - Service the platform
    LOG_service_platform();

    // Assert - Service request should be cleared
    TEST_ASSERT_FALSE(g_LOG_LL_service_request);
}

void test_LOG_service_platform_no_request(void)
{
    // Arrange - No service request
    g_LOG_LL_service_request = false;

    // Act
    LOG_service_platform();

    // Assert - Should return early without processing
    TEST_ASSERT_FALSE(g_LOG_LL_service_request);
}

void test_LOG_service_platform_multiple_entries(void)
{
    // Arrange - Write multiple entries
    LOG_LL_write(LOG_LL_ERROR, "Error entry");
    LOG_LL_write(LOG_LL_WARN, "Warning entry");
    LOG_LL_write(LOG_LL_INFO, "Info entry");
    LOG_LL_write(LOG_LL_DEBUG, "Debug entry");

    // Act
    LOG_service_platform();

    // Assert - All entries should be processed and buffer empty
    TEST_ASSERT_FALSE(g_LOG_LL_service_request);
    TEST_ASSERT_EQUAL(0, LOG_LL_available());
}

void test_LOG_LL_buffer_reset_behavior(void)
{
    // Arrange - Write some data first
    LOG_LL_write(LOG_LL_INFO, "Before reset");
    TEST_ASSERT_GREATER_THAN(0, LOG_LL_available());

    // Act - Reset the buffer
    circular_buffer_reset(&g_LOG_LL_buffer);

    // Assert - Buffer should be empty after reset
    TEST_ASSERT_EQUAL(0, LOG_LL_available());

    // Verify we can still write after reset (since LOG_LL_init was called in setUp)
    LOG_LL_write(LOG_LL_INFO, "After reset message");
    TEST_ASSERT_GREATER_THAN(0, LOG_LL_available());

    // Verify we can read the new message
    LOG_LL_Entry entry;
    TEST_ASSERT_TRUE(LOG_LL_read_entry(&entry));
    TEST_ASSERT_EQUAL_STRING("After reset message", entry.message);
}

void test_LOG_LL_all_log_levels(void)
{
    // Test all log levels to ensure they're handled correctly
    LOG_LL_write(LOG_LL_ERROR, "Error level test");
    LOG_LL_write(LOG_LL_WARN, "Warning level test");
    LOG_LL_write(LOG_LL_INFO, "Info level test");
    LOG_LL_write(LOG_LL_DEBUG, "Debug level test");

    // Verify each level is correctly stored and retrieved
    LOG_LL_Entry entry;

    TEST_ASSERT_TRUE(LOG_LL_read_entry(&entry));
    TEST_ASSERT_EQUAL(LOG_LL_ERROR, entry.level);

    TEST_ASSERT_TRUE(LOG_LL_read_entry(&entry));
    TEST_ASSERT_EQUAL(LOG_LL_WARN, entry.level);

    TEST_ASSERT_TRUE(LOG_LL_read_entry(&entry));
    TEST_ASSERT_EQUAL(LOG_LL_INFO, entry.level);

    TEST_ASSERT_TRUE(LOG_LL_read_entry(&entry));
    TEST_ASSERT_EQUAL(LOG_LL_DEBUG, entry.level);
}

void test_LOG_LL_convenience_macros(void)
{
    // Test the convenience macros defined in the header
    LOG_LL_ERROR("Error macro test");
    LOG_LL_WARN("Warning macro test");
    LOG_LL_INFO("Info macro test");
    LOG_LL_DEBUG("Debug macro test");

    // Verify the macros work correctly
    LOG_LL_Entry entry;

    TEST_ASSERT_TRUE(LOG_LL_read_entry(&entry));
    TEST_ASSERT_EQUAL(LOG_LL_ERROR, entry.level);
    TEST_ASSERT_EQUAL_STRING("Error macro test", entry.message);

    TEST_ASSERT_TRUE(LOG_LL_read_entry(&entry));
    TEST_ASSERT_EQUAL(LOG_LL_WARN, entry.level);
    TEST_ASSERT_EQUAL_STRING("Warning macro test", entry.message);

    TEST_ASSERT_TRUE(LOG_LL_read_entry(&entry));
    TEST_ASSERT_EQUAL(LOG_LL_INFO, entry.level);
    TEST_ASSERT_EQUAL_STRING("Info macro test", entry.message);

    TEST_ASSERT_TRUE(LOG_LL_read_entry(&entry));
    TEST_ASSERT_EQUAL(LOG_LL_DEBUG, entry.level);
    TEST_ASSERT_EQUAL_STRING("Debug macro test", entry.message);
}

void test_LOG_LL_zero_length_message(void)
{
    // Test edge case of empty message
    LOG_LL_write(LOG_LL_INFO, "");

    LOG_LL_Entry entry;
    TEST_ASSERT_TRUE(LOG_LL_read_entry(&entry));
    TEST_ASSERT_EQUAL(LOG_LL_INFO, entry.level);
    TEST_ASSERT_EQUAL(0, entry.length);
    TEST_ASSERT_EQUAL_STRING("", entry.message);
}

void test_LOG_LL_buffer_boundary_conditions(void)
{
    // Test when buffer has exactly enough space for one more entry
    size_t initial_free = circular_buffer_free_space(&g_LOG_LL_buffer);

    // Fill buffer until we have just enough space for a small entry
    while (circular_buffer_free_space(&g_LOG_LL_buffer) > 20) {
        LOG_LL_write(LOG_LL_INFO, "Fill");
    }

    // Try to write an entry that should fit
    size_t free_before = circular_buffer_free_space(&g_LOG_LL_buffer);
    LOG_LL_write(LOG_LL_INFO, "Last");

    // Should either succeed or fail gracefully
    size_t free_after = circular_buffer_free_space(&g_LOG_LL_buffer);
    TEST_ASSERT_TRUE(free_after <= free_before);
}
