#include "unity.h"

#include "util.h"
#include <string.h>

#include "logging.c" // Include the implementation for testing

void setUp(void)
{
    // Initialize logging
    LOG_init();
}

void tearDown(void)
{
    // Reset logging state
    circular_buffer_reset(&g_log_state.buffer);
}

void test_LOG_init(void)
{
    // Check if the log buffer is initialized correctly
    TEST_ASSERT_NOT_NULL(g_log_state.buffer.buffer);
    TEST_ASSERT_EQUAL(0, g_log_state.buffer.head);
    TEST_ASSERT_EQUAL(0, g_log_state.buffer.tail);
    TEST_ASSERT_EQUAL(LOG_BUFFER_SIZE, g_log_state.buffer.size);
}

void test_LOG_write(void)
{
    // Arrange
    char const *test_message = "Test log message";

    // Act
    int bytes_written = LOG_write(LOG_LEVEL_DEBUG, test_message);

    // Assert
    LOG_Entry entry;
    circular_buffer_read(
        &g_log_state.buffer, (uint8_t *)&entry, sizeof(entry)
    );

    TEST_ASSERT_EQUAL(LOG_LEVEL_DEBUG, entry.level);
    TEST_ASSERT_EQUAL_STRING(entry.message, test_message);
    TEST_ASSERT_EQUAL(strlen(test_message), entry.length);
    TEST_ASSERT_EQUAL(
        bytes_written,
        sizeof(entry.level) + sizeof(entry.length) + entry.length + 1
    );
    // Verify the message is null-terminated
    TEST_ASSERT_EQUAL('\0', entry.message[entry.length]);
}

void test_LOG_available(void)
{
    // Arrange
    char const *test_message = "Test log message";

    // Act
    int bytes_written = LOG_write(LOG_LEVEL_DEBUG, test_message);
    size_t available = LOG_available();

    // Assert
    TEST_ASSERT_GREATER_THAN(0, bytes_written);
    TEST_ASSERT_EQUAL(available, bytes_written);
}

// Test LOG_task processes at most 8 entries per call
void test_LOG_task_partial_processing(void)
{
    // Arrange: Write more than 8 entries
    const int total_entries = 12;
    for (int i = 0; i < total_entries; i++) {
        int bytes_written = LOG_write(LOG_LEVEL_INFO, "Test entry %d", i);
        TEST_ASSERT_GREATER_THAN(0, bytes_written);
    }

    // Act: Call service once
    LOG_task();

    // Assert: Only 8 processed, 4 remain
    TEST_ASSERT_GREATER_THAN(0, LOG_available());

    // Act: Call service again
    LOG_task();

    // Assert: Remaining 4 processed, buffer empty
    TEST_ASSERT_EQUAL(0, LOG_available());
}

void test_LOG_write_multiple_entries(void)
{
    // Arrange & Act
    int bytes1 = LOG_write(LOG_LEVEL_ERROR, "Error message");
    int bytes2 = LOG_write(LOG_LEVEL_WARN, "Warning message");
    int bytes3 = LOG_write(LOG_LEVEL_INFO, "Info message");
    int bytes4 = LOG_write(LOG_LEVEL_DEBUG, "Debug message");

    // Assert - All writes should succeed
    TEST_ASSERT_GREATER_THAN(0, bytes1);
    TEST_ASSERT_GREATER_THAN(0, bytes2);
    TEST_ASSERT_GREATER_THAN(0, bytes3);
    TEST_ASSERT_GREATER_THAN(0, bytes4);
    TEST_ASSERT_GREATER_THAN(0, LOG_available());

    // Verify we can read multiple entries
    LOG_Entry entry;
    TEST_ASSERT_TRUE(read_entry(&entry));
    TEST_ASSERT_EQUAL(LOG_LEVEL_ERROR, entry.level);
    TEST_ASSERT_EQUAL_STRING("Error message", entry.message);

    TEST_ASSERT_TRUE(read_entry(&entry));
    TEST_ASSERT_EQUAL(LOG_LEVEL_WARN, entry.level);
    TEST_ASSERT_EQUAL_STRING("Warning message", entry.message);

    TEST_ASSERT_TRUE(read_entry(&entry));
    TEST_ASSERT_EQUAL(LOG_LEVEL_INFO, entry.level);
    TEST_ASSERT_EQUAL_STRING("Info message", entry.message);

    TEST_ASSERT_TRUE(read_entry(&entry));
    TEST_ASSERT_EQUAL(LOG_LEVEL_DEBUG, entry.level);
    TEST_ASSERT_EQUAL_STRING("Debug message", entry.message);
}

void test_LOG_write_with_format_string(void)
{
    // Arrange
    int value = 42;
    char const *string = "test";

    // Act
    int bytes_written = LOG_write(LOG_LEVEL_INFO, "Value: %d, String: %s", value, string);

    // Assert
    TEST_ASSERT_GREATER_THAN(0, bytes_written);
    LOG_Entry entry;
    TEST_ASSERT_TRUE(read_entry(&entry));
    TEST_ASSERT_EQUAL(LOG_LEVEL_INFO, entry.level);
    TEST_ASSERT_EQUAL_STRING("Value: 42, String: test", entry.message);
}

void test_LOG_write_long_message(void)
{
    // Arrange - Create a message longer than the buffer can hold
    char long_message[LOG_MAX_MESSAGE_SIZE + 50];
    memset(long_message, 'A', sizeof(long_message) - 1);
    long_message[sizeof(long_message) - 1] = '\0';

    // Act
    int bytes_written = LOG_write(LOG_LEVEL_INFO, "%s", long_message);

    // Assert - Write should succeed but message should be truncated
    TEST_ASSERT_GREATER_THAN(0, bytes_written);
    LOG_Entry entry;
    TEST_ASSERT_TRUE(read_entry(&entry));
    TEST_ASSERT_EQUAL(LOG_LEVEL_INFO, entry.level);
    TEST_ASSERT_EQUAL(LOG_MAX_MESSAGE_SIZE - 1, entry.length);
    TEST_ASSERT_EQUAL('\0', entry.message[entry.length]);
}

void test_LOG_read_entry_empty_buffer(void)
{
    // Arrange - Buffer is empty after setUp/tearDown

    // Act
    LOG_Entry entry;
    bool result = read_entry(&entry);

    // Assert
    TEST_ASSERT_FALSE(result);
}

void test_LOG_read_entry_null_pointer(void)
{
    // Arrange
    int bytes_written = LOG_write(LOG_LEVEL_INFO, "Test message");
    TEST_ASSERT_GREATER_THAN(0, bytes_written);

    // Act & Assert
    bool result = read_entry(NULL);
    TEST_ASSERT_FALSE(result);
}

void test_LOG_buffer_overflow(void)
{
    // Arrange - Write messages until buffer is filled
    size_t entries_written = 0;
    while (LOG_write(LOG_LEVEL_INFO, "Entry %zu", entries_written++) > 0);

    // Assert - First entry should be available
    LOG_Entry entry;
    TEST_ASSERT_TRUE(read_entry(&entry));
    TEST_ASSERT_EQUAL_STRING("Entry 0", entry.message);
    // Final entry written should be dropped
    // Final entry in buffer should be the second to last written
    while (read_entry(&entry));
    char final_entry_message_in_buffer[LOG_MAX_MESSAGE_SIZE];
    snprintf(
        final_entry_message_in_buffer,
        sizeof(final_entry_message_in_buffer),
        "Entry %zu",
        entries_written - 2
    );
    TEST_ASSERT_EQUAL_STRING(final_entry_message_in_buffer, entry.message);
}

void test_LOG_task_multiple_entries(void)
{
    // Arrange - Write multiple entries
    int bytes1 = LOG_write(LOG_LEVEL_ERROR, "Error entry");
    int bytes2 = LOG_write(LOG_LEVEL_WARN, "Warning entry");
    int bytes3 = LOG_write(LOG_LEVEL_INFO, "Info entry");
    int bytes4 = LOG_write(LOG_LEVEL_DEBUG, "Debug entry");

    // Assert all writes succeeded
    TEST_ASSERT_GREATER_THAN(0, bytes1);
    TEST_ASSERT_GREATER_THAN(0, bytes2);
    TEST_ASSERT_GREATER_THAN(0, bytes3);
    TEST_ASSERT_GREATER_THAN(0, bytes4);

    // Act
    LOG_task();

    // Assert - All entries should be processed and buffer empty
    TEST_ASSERT_EQUAL(0, LOG_available());
}

void test_LOG_buffer_reset_behavior(void)
{
    // Arrange - Write some data first
    int bytes_written1 = LOG_write(LOG_LEVEL_INFO, "Before reset");
    TEST_ASSERT_GREATER_THAN(0, bytes_written1);
    TEST_ASSERT_GREATER_THAN(0, LOG_available());

    // Act - Reset the buffer
    circular_buffer_reset(&g_log_state.buffer);

    // Assert - Buffer should be empty after reset
    TEST_ASSERT_EQUAL(0, LOG_available());

    // Verify we can still write after reset (since LOG_init was called in setUp)
    int bytes_written2 = LOG_write(LOG_LEVEL_INFO, "After reset message");
    TEST_ASSERT_GREATER_THAN(0, bytes_written2);
    TEST_ASSERT_GREATER_THAN(0, LOG_available());

    // Verify we can read the new message
    LOG_Entry entry;
    TEST_ASSERT_TRUE(read_entry(&entry));
    TEST_ASSERT_EQUAL_STRING("After reset message", entry.message);
}

void test_LOG_all_log_levels(void)
{
    // Test all log levels to ensure they're handled correctly
    int bytes1 = LOG_write(LOG_LEVEL_ERROR, "Error level test");
    int bytes2 = LOG_write(LOG_LEVEL_WARN, "Warning level test");
    int bytes3 = LOG_write(LOG_LEVEL_INFO, "Info level test");
    int bytes4 = LOG_write(LOG_LEVEL_DEBUG, "Debug level test");

    // Assert all writes succeeded
    TEST_ASSERT_GREATER_THAN(0, bytes1);
    TEST_ASSERT_GREATER_THAN(0, bytes2);
    TEST_ASSERT_GREATER_THAN(0, bytes3);
    TEST_ASSERT_GREATER_THAN(0, bytes4);

    // Verify each level is correctly stored and retrieved
    LOG_Entry entry;

    TEST_ASSERT_TRUE(read_entry(&entry));
    TEST_ASSERT_EQUAL(LOG_LEVEL_ERROR, entry.level);

    TEST_ASSERT_TRUE(read_entry(&entry));
    TEST_ASSERT_EQUAL(LOG_LEVEL_WARN, entry.level);

    TEST_ASSERT_TRUE(read_entry(&entry));
    TEST_ASSERT_EQUAL(LOG_LEVEL_INFO, entry.level);

    TEST_ASSERT_TRUE(read_entry(&entry));
    TEST_ASSERT_EQUAL(LOG_LEVEL_DEBUG, entry.level);
}

void test_LOG_convenience_macros(void)
{
    // Test the convenience macros defined in the header
    LOG_ERROR("Error macro test");
    LOG_WARN("Warning macro test");
    LOG_INFO("Info macro test");
    LOG_DEBUG("Debug macro test");

    // Verify the macros work correctly
    LOG_Entry entry;

    TEST_ASSERT_TRUE(read_entry(&entry));
    TEST_ASSERT_EQUAL(LOG_LEVEL_ERROR, entry.level);
    TEST_ASSERT_EQUAL_STRING("Error macro test", entry.message);

    TEST_ASSERT_TRUE(read_entry(&entry));
    TEST_ASSERT_EQUAL(LOG_LEVEL_WARN, entry.level);
    TEST_ASSERT_EQUAL_STRING("Warning macro test", entry.message);

    TEST_ASSERT_TRUE(read_entry(&entry));
    TEST_ASSERT_EQUAL(LOG_LEVEL_INFO, entry.level);
    TEST_ASSERT_EQUAL_STRING("Info macro test", entry.message);

    TEST_ASSERT_TRUE(read_entry(&entry));
    TEST_ASSERT_EQUAL(LOG_LEVEL_DEBUG, entry.level);
    TEST_ASSERT_EQUAL_STRING("Debug macro test", entry.message);
}

// Test edge case of empty message
void test_LOG_zero_length_message(void)
{
    // Act
    int bytes_written = LOG_write(LOG_LEVEL_INFO, "");

    // Assert - Write should succeed even for empty message
    TEST_ASSERT_GREATER_THAN(0, bytes_written);
    LOG_Entry entry;
    TEST_ASSERT_TRUE(read_entry(&entry));
    TEST_ASSERT_EQUAL(LOG_LEVEL_INFO, entry.level);
    TEST_ASSERT_EQUAL(0, entry.length);
    TEST_ASSERT_EQUAL_STRING("", entry.message);
}

// Test that buffer can be partially read to make space for new entries
void test_LOG_buffer_partial_read(void)
{
    // Arrange - Write messages until buffer is filled
    size_t entries_written = 0;
    while (LOG_write(LOG_LEVEL_INFO, "Entry %zu", entries_written) > 0) {
        entries_written++;
    }

    // Act - Read back half the entries to make space
    size_t entries_to_read = entries_written / 2;
    for (size_t i = 0; i < entries_to_read; i++) {
        LOG_Entry entry;
        TEST_ASSERT_TRUE(read_entry(&entry));
    }

    // Assert - Buffer should still have remaining entries
    TEST_ASSERT_GREATER_THAN(0, LOG_available());

    // Verify we can write new entries after partial read
    TEST_ASSERT_GREATER_THAN(0, LOG_write(LOG_LEVEL_INFO, "New entry after partial read"));
}

// Test buffer wraparound detection
void test_LOG_buffer_wraparound_detection(void)
{
    // Arrange - Fill buffer completely
    size_t entries_written = 0;
    while (LOG_write(LOG_LEVEL_INFO, "Fill entry %zu", entries_written) > 0) {
        entries_written++;
    }

    // Read half the entries to make space
    size_t entries_to_read = entries_written / 2;
    for (size_t i = 0; i < entries_to_read; i++) {
        LOG_Entry entry;
        read_entry(&entry);
    }

    // Act - Write one more message to cause wraparound
    LOG_write(LOG_LEVEL_INFO, "Wraparound entry");

    // Assert - Buffer should show wraparound condition (head < tail)
    TEST_ASSERT_LESS_THAN(g_log_state.buffer.tail, g_log_state.buffer.head);
}

// Test data integrity before wraparound
void test_LOG_buffer_data_integrity_before_wraparound(void)
{
    // Arrange - Fill buffer with numbered entries
    size_t entries_written = 0;
    while (LOG_write(LOG_LEVEL_INFO, "Entry %zu", entries_written) > 0) {
        entries_written++;
    }

    // Read half the entries
    size_t entries_to_read = entries_written / 2;
    for (size_t i = 0; i < entries_to_read; i++) {
        LOG_Entry entry;
        read_entry(&entry);
    }

    // Act & Assert - Remaining entries should be readable with correct content
    LOG_Entry entry;
    for (size_t i = entries_to_read; i < entries_written; i++) {
        char expected_message[LOG_MAX_MESSAGE_SIZE];
        snprintf(expected_message, sizeof(expected_message), "Entry %zu", i);

        TEST_ASSERT_TRUE(read_entry(&entry));
        TEST_ASSERT_EQUAL(LOG_LEVEL_INFO, entry.level);
        TEST_ASSERT_EQUAL_STRING(expected_message, entry.message);
    }
}

// Test data integrity after wraparound
void test_LOG_buffer_data_integrity_after_wraparound(void)
{
    // Arrange - Fill buffer, then clear some space
    size_t entries_written = 0;
    while (LOG_write(LOG_LEVEL_INFO, "Initial entry %zu", entries_written) > 0) {
        entries_written++;
    }

    // Read half the entries to make space
    size_t entries_to_read = entries_written / 2;
    for (size_t i = 0; i < entries_to_read; i++) {
        LOG_Entry entry;
        read_entry(&entry);
    }

    // Act - Write new numbered entries after wraparound
    size_t new_entries_written = 0;
    while (LOG_write(LOG_LEVEL_INFO, "New Entry %zu", new_entries_written) > 0) {
        new_entries_written++;
    }

    // Skip remaining pre-wraparound entries
    LOG_Entry entry;
    while (read_entry(&entry)) {
        if (strstr(entry.message, "New Entry") != NULL) {
            // Found first new entry - it's now in the entry variable
            break;
        }
    }

    // Assert - New entries should be readable with correct content
    for (size_t i = 0; i < new_entries_written; i++) {
        char expected_message[LOG_MAX_MESSAGE_SIZE];
        snprintf(expected_message, sizeof(expected_message), "New Entry %zu", i);

        if (i == 0) {
            // First entry was already read above
            TEST_ASSERT_EQUAL_STRING(expected_message, entry.message);
        } else {
            TEST_ASSERT_TRUE(read_entry(&entry));
            TEST_ASSERT_EQUAL_STRING(expected_message, entry.message);
        }
    }
}
