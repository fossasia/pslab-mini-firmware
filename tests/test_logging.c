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
