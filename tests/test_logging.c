#include "unity.h"

#include "logging.h"
#include "platform_logging.h"
#include "util.h"
#include <string.h>

// Test fixtures
static PLATFORM_LogEntry g_test_entry;

void setUp(void)
{
    // Initialize platform logging
    PLATFORM_log_init();
}

void tearDown(void)
{
    // Reset platform log state
    circular_buffer_reset(&g_platform_log_buffer);
}

void test_PLATFORM_log_init(void)
{
    // Check if the log buffer is initialized correctly
    TEST_ASSERT_NOT_NULL(g_platform_log_buffer.buffer);
    TEST_ASSERT_EQUAL(0, g_platform_log_buffer.head);
    TEST_ASSERT_EQUAL(0, g_platform_log_buffer.tail);
    TEST_ASSERT_EQUAL(PLATFORM_LOG_BUFFER_SIZE, g_platform_log_buffer.size);
}

void test_PLATFORM_log_write(void)
{
    // Arrange
    char const *test_message = "Test log message";

    // Act
    PLATFORM_log_write(PLATFORM_LOG_DEBUG, test_message);

    // Assert
    TEST_ASSERT_EQUAL(
        sizeof(g_test_entry.level)
        + sizeof(g_test_entry.length)
        + strlen(test_message) + 1, // +1 for null terminator
        g_platform_log_buffer.head
    );
    TEST_ASSERT_EQUAL_MEMORY(
        test_message,
        g_platform_log_buffer.buffer
        + g_platform_log_buffer.tail
        + sizeof(g_test_entry.level)
        + sizeof(g_test_entry.length),
        strlen(test_message)
    );
}

void test_PLATFORM_log_read_entry(void)
{
    // Arrange
    PLATFORM_LogEntry entry;
    entry.level = PLATFORM_LOG_INFO;
    entry.length = 20;
    strcpy(entry.message, "Test log message");

    // Write to the platform log buffer
    circular_buffer_write(
        &g_platform_log_buffer,
        (uint8_t *)&entry,
        sizeof(entry.level) + sizeof(entry.length) + entry.length + 1
    );

    // Act
    bool result = platform_log_read_entry(&entry);

    // Assert
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(PLATFORM_LOG_INFO, entry.level);
    TEST_ASSERT_EQUAL_STRING("Test log message", entry.message);
}

void test_PLATFORM_log_available(void)
{
    // Arrange
    char const *test_message = "Test log message";
    PLATFORM_log_write(PLATFORM_LOG_DEBUG, test_message);

    // Act
    size_t available = PLATFORM_log_available();

    // Assert
    TEST_ASSERT_GREATER_THAN(0, available);
}

void test_LOG_service_platform(void)
{
    // Arrange
    g_platform_log_service_request = true;
    // Fill the log buffer with a test entry
    PLATFORM_log_write(PLATFORM_LOG_INFO, "Test log entry");

    // Act
    LOG_service_platform();

    // Assert - Should process the log entry and reset the request flag
    TEST_ASSERT_FALSE(g_platform_log_service_request);
}
