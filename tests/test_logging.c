#include <string.h>

#include "unity.h"

#include "util/logging.h"
#include "util/util.h"


// Forward declarations of implementation-internals
struct LOG_Handle {
    CircularBuffer buffer;
    uint8_t buffer_data[LOG_BUFFER_SIZE];
    bool initialized;
};

static struct LOG_Handle *g_log_handle = nullptr;

void setUp(void)
{
    // Initialize logging
    g_log_handle = LOG_init();
}

void tearDown(void)
{
    // Reset logging state
    LOG_deinit(g_log_handle);
}

void test_LOG_init(void)
{
    // Check if the log buffer is initialized correctly
    TEST_ASSERT_NOT_NULL(g_log_handle->buffer.buffer);
    TEST_ASSERT_EQUAL(0, g_log_handle->buffer.head);
    TEST_ASSERT_EQUAL(0, g_log_handle->buffer.tail);
    TEST_ASSERT_EQUAL(LOG_BUFFER_SIZE, g_log_handle->buffer.size);
}

void test_LOG_init_already_initialized(void)
{
    // Arrange - Call init again
    LOG_Handle *handle = LOG_init();

    // Assert - Should return the same handle
    TEST_ASSERT_EQUAL_PTR(g_log_handle, handle);
}

void test_LOG_deinit(void)
{
    // Arrange
    LOG_Handle *handle = g_log_handle;

    // Act
    LOG_deinit(handle);

    // Assert - Check if handle is still valid after deinit
    TEST_ASSERT_NOT_NULL(handle);
    TEST_ASSERT_FALSE(handle->initialized);
    TEST_ASSERT_EQUAL(0, circular_buffer_available(&handle->buffer));
}

void test_LOG_deinit_invalid_handle(void)
{
    // Arrange
    LOG_Handle invalid_handle = { .initialized = true };

    // Act - Attempt to deinitialize an invalid handle
    LOG_deinit(&invalid_handle);

    // Assert - Should not crash, handle remains unchanged
    TEST_ASSERT_TRUE(invalid_handle.initialized);
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
        &g_log_handle->buffer, (uint8_t *)&entry, sizeof(entry)
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
    TEST_ASSERT_TRUE(LOG_read_entry(&entry));
    TEST_ASSERT_EQUAL(LOG_LEVEL_ERROR, entry.level);
    TEST_ASSERT_EQUAL_STRING("Error message", entry.message);

    TEST_ASSERT_TRUE(LOG_read_entry(&entry));
    TEST_ASSERT_EQUAL(LOG_LEVEL_WARN, entry.level);
    TEST_ASSERT_EQUAL_STRING("Warning message", entry.message);

    TEST_ASSERT_TRUE(LOG_read_entry(&entry));
    TEST_ASSERT_EQUAL(LOG_LEVEL_INFO, entry.level);
    TEST_ASSERT_EQUAL_STRING("Info message", entry.message);

    TEST_ASSERT_TRUE(LOG_read_entry(&entry));
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
    TEST_ASSERT_TRUE(LOG_read_entry(&entry));
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
    TEST_ASSERT_TRUE(LOG_read_entry(&entry));
    TEST_ASSERT_EQUAL(LOG_LEVEL_INFO, entry.level);
    TEST_ASSERT_EQUAL(LOG_MAX_MESSAGE_SIZE - 1, entry.length);
    TEST_ASSERT_EQUAL('\0', entry.message[entry.length]);
}

void test_LOG_write_null_message(void)
{
    // Act - Attempt to write a null message
    int bytes_written = LOG_write(LOG_LEVEL_INFO, NULL);

    // Assert - Should return -1 for invalid format string
    TEST_ASSERT_EQUAL(-1, bytes_written);
}

void test_LOG_write_invalid_level(void)
{
    // Act - Attempt to write with an invalid log level
    int bytes_written = LOG_write((LOG_Level)999, "Invalid level test");

    // Assert - Should return -1 for invalid log level
    TEST_ASSERT_EQUAL(-1, bytes_written);
}

void test_LOG_read_entry_success(void)
{
    // Arrange - Write a message first
    int bytes_written = LOG_write(LOG_LEVEL_INFO, "Test message");
    TEST_ASSERT_GREATER_THAN(0, bytes_written);

    // Act
    LOG_Entry entry;
    bool result = LOG_read_entry(&entry);

    // Assert
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(LOG_LEVEL_INFO, entry.level);
    TEST_ASSERT_EQUAL_STRING("Test message", entry.message);
}

void test_LOG_read_entry_empty_buffer(void)
{
    // Arrange - Buffer is empty after setUp/tearDown

    // Act
    LOG_Entry entry;
    bool result = LOG_read_entry(&entry);

    // Assert
    TEST_ASSERT_FALSE(result);
}

void test_LOG_read_entry_null_pointer(void)
{
    // Arrange
    int bytes_written = LOG_write(LOG_LEVEL_INFO, "Test message");
    TEST_ASSERT_GREATER_THAN(0, bytes_written);

    // Act & Assert
    bool result = LOG_read_entry(NULL);
    TEST_ASSERT_FALSE(result);
}

void test_LOG_buffer_overflow(void)
{
    // Arrange - Write messages until buffer is filled
    size_t entries_written = 0;
    while (LOG_write(LOG_LEVEL_INFO, "LOG_Entry %zu", entries_written++) > 0);

    // Assert - First entry should be available
    LOG_Entry entry;
    TEST_ASSERT_TRUE(LOG_read_entry(&entry));
    TEST_ASSERT_EQUAL_STRING("LOG_Entry 0", entry.message);
    // Final entry written should be dropped
    // Final entry in buffer should be the second to last written
    while (LOG_read_entry(&entry));
    char final_entry_message_in_buffer[LOG_MAX_MESSAGE_SIZE];
    snprintf(
        final_entry_message_in_buffer,
        sizeof(final_entry_message_in_buffer),
        "LOG_Entry %zu",
        entries_written - 2
    );
    TEST_ASSERT_EQUAL_STRING(final_entry_message_in_buffer, entry.message);
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

void test_LOG_available_empty_buffer(void)
{
    // Act
    size_t available = LOG_available();

    // Assert - Should be 0 after setUp
    TEST_ASSERT_EQUAL(0, available);
}

// Test LOG_task processes fewer entries than available
void test_LOG_task_partial_processing(void)
{
    // Arrange: Write several entries
    const int total_entries = 12;
    for (int i = 0; i < total_entries; i++) {
        int bytes_written = LOG_write(LOG_LEVEL_INFO, "Test entry %d", i);
        TEST_ASSERT_GREATER_THAN(0, bytes_written);
    }

    // Act: Process fewer entries than available
    int processed = LOG_task(8);

    // Assert: Only 8 processed, 4 remain
    TEST_ASSERT_EQUAL(8, processed);
    TEST_ASSERT_GREATER_THAN(0, LOG_available());

    // Act: Call service again
    processed += LOG_task(8);

    // Assert: Remaining 4 processed, buffer empty
    TEST_ASSERT_EQUAL(12, processed);
    TEST_ASSERT_EQUAL(0, LOG_available());
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
    int processed = LOG_task(4);

    // Assert - All entries should be processed and buffer empty
    TEST_ASSERT_EQUAL(4, processed);
    TEST_ASSERT_EQUAL(0, LOG_available());
}

void test_LOG_task_no_entries(void)
{
    // Act - Call task with no entries
    int processed = LOG_task(5);

    // Assert - Should process 0 entries
    TEST_ASSERT_EQUAL(0, processed);
    TEST_ASSERT_EQUAL(0, LOG_available());
}

void test_LOG_buffer_reset_behavior(void)
{
    // Arrange - Write some data first
    int bytes_written1 = LOG_write(LOG_LEVEL_INFO, "Before reset");
    TEST_ASSERT_GREATER_THAN(0, bytes_written1);
    TEST_ASSERT_GREATER_THAN(0, LOG_available());

    // Act - Reset the buffer
    circular_buffer_reset(&g_log_handle->buffer);

    // Assert - Buffer should be empty after reset
    TEST_ASSERT_EQUAL(0, LOG_available());

    // Verify we can still write after reset (since LOG_init was called in setUp)
    int bytes_written2 = LOG_write(LOG_LEVEL_INFO, "After reset message");
    TEST_ASSERT_GREATER_THAN(0, bytes_written2);
    TEST_ASSERT_GREATER_THAN(0, LOG_available());

    // Verify we can read the new message
    LOG_Entry entry;
    TEST_ASSERT_TRUE(LOG_read_entry(&entry));
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

    TEST_ASSERT_TRUE(LOG_read_entry(&entry));
    TEST_ASSERT_EQUAL(LOG_LEVEL_ERROR, entry.level);

    TEST_ASSERT_TRUE(LOG_read_entry(&entry));
    TEST_ASSERT_EQUAL(LOG_LEVEL_WARN, entry.level);

    TEST_ASSERT_TRUE(LOG_read_entry(&entry));
    TEST_ASSERT_EQUAL(LOG_LEVEL_INFO, entry.level);

    TEST_ASSERT_TRUE(LOG_read_entry(&entry));
    TEST_ASSERT_EQUAL(LOG_LEVEL_DEBUG, entry.level);
}

void test_LOG_convenience_macros(void)
{
    // Test the convenience macros defined in the header and verify compile-time filtering
    // This test verifies that macros are properly filtered at compile time
    // based on LOG_COMPILE_TIME_LEVEL setting

    // Reset buffer to ensure clean state
    circular_buffer_reset(&g_log_handle->buffer);

    // Test all convenience macros - only those at or above compile-time level should write
    LOG_ERROR("Error macro test");
    LOG_WARN("Warning macro test");
    LOG_INFO("Info macro test");
    LOG_DEBUG("Debug macro test");

    // Verify macros work according to the compile-time level setting
    LOG_Entry entry;
    int expected_entries = 0;

    // Check LOG_ERROR (always enabled when LOG_COMPILE_TIME_LEVEL >= 0)
#if LOG_COMPILE_TIME_LEVEL >= 0
    expected_entries++;
    TEST_ASSERT_TRUE(LOG_read_entry(&entry));
    TEST_ASSERT_EQUAL(LOG_LEVEL_ERROR, entry.level);
    TEST_ASSERT_EQUAL_STRING("Error macro test", entry.message);
#endif

    // Check LOG_WARN (enabled when LOG_COMPILE_TIME_LEVEL >= 1)
#if LOG_COMPILE_TIME_LEVEL >= 1
    expected_entries++;
    TEST_ASSERT_TRUE(LOG_read_entry(&entry));
    TEST_ASSERT_EQUAL(LOG_LEVEL_WARN, entry.level);
    TEST_ASSERT_EQUAL_STRING("Warning macro test", entry.message);
#endif

    // Check LOG_INFO (enabled when LOG_COMPILE_TIME_LEVEL >= 2)
#if LOG_COMPILE_TIME_LEVEL >= 2
    expected_entries++;
    TEST_ASSERT_TRUE(LOG_read_entry(&entry));
    TEST_ASSERT_EQUAL(LOG_LEVEL_INFO, entry.level);
    TEST_ASSERT_EQUAL_STRING("Info macro test", entry.message);
#endif

    // Check LOG_DEBUG (enabled when LOG_COMPILE_TIME_LEVEL >= 3)
#if LOG_COMPILE_TIME_LEVEL >= 3
    expected_entries++;
    TEST_ASSERT_TRUE(LOG_read_entry(&entry));
    TEST_ASSERT_EQUAL(LOG_LEVEL_DEBUG, entry.level);
    TEST_ASSERT_EQUAL_STRING("Debug macro test", entry.message);
#endif

    // No more entries should be available
    TEST_ASSERT_FALSE(LOG_read_entry(&entry));

    // Buffer should be empty now
    TEST_ASSERT_EQUAL(0, LOG_available());

    // Verify we got at least one entry (ERROR should always be enabled)
    TEST_ASSERT_GREATER_THAN(0, expected_entries);

    // Verify that the logging system still works for enabled levels
    // Use direct LOG_write call to confirm buffer functionality
    int bytes_written = LOG_write(LOG_LEVEL_ERROR, "Direct write test");
    TEST_ASSERT_GREATER_THAN(0, bytes_written);

    // Should be able to read this direct write
    TEST_ASSERT_TRUE(LOG_read_entry(&entry));
    TEST_ASSERT_EQUAL(LOG_LEVEL_ERROR, entry.level);
    TEST_ASSERT_EQUAL_STRING("Direct write test", entry.message);
}

// Test edge case of empty message
void test_LOG_zero_length_message(void)
{
    // Act
    int bytes_written = LOG_write(LOG_LEVEL_INFO, "");

    // Assert - Write should succeed even for empty message
    TEST_ASSERT_GREATER_THAN(0, bytes_written);
    LOG_Entry entry;
    TEST_ASSERT_TRUE(LOG_read_entry(&entry));
    TEST_ASSERT_EQUAL(LOG_LEVEL_INFO, entry.level);
    TEST_ASSERT_EQUAL(0, entry.length);
    TEST_ASSERT_EQUAL_STRING("", entry.message);
}

// Test that buffer can be partially read to make space for new entries
void test_LOG_buffer_partial_read(void)
{
    // Arrange - Write messages until buffer is filled
    size_t entries_written = 0;
    while (LOG_write(LOG_LEVEL_INFO, "LOG_Entry %zu", entries_written) > 0) {
        entries_written++;
    }

    // Act - Read back half the entries to make space
    size_t entries_to_read = entries_written / 2;
    for (size_t i = 0; i < entries_to_read; i++) {
        LOG_Entry entry;
        TEST_ASSERT_TRUE(LOG_read_entry(&entry));
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
        LOG_read_entry(&entry);
    }

    // Act - Write one more message to cause wraparound
    LOG_write(LOG_LEVEL_INFO, "Wraparound entry");

    // Assert - Buffer should show wraparound condition (head < tail)
    TEST_ASSERT_LESS_THAN(g_log_handle->buffer.tail, g_log_handle->buffer.head);
}

// Test data integrity before wraparound
void test_LOG_buffer_data_integrity_before_wraparound(void)
{
    // Arrange - Fill buffer with numbered entries
    size_t entries_written = 0;
    while (LOG_write(LOG_LEVEL_INFO, "LOG_Entry %zu", entries_written) > 0) {
        entries_written++;
    }

    // Read half the entries
    size_t entries_to_read = entries_written / 2;
    for (size_t i = 0; i < entries_to_read; i++) {
        LOG_Entry entry;
        LOG_read_entry(&entry);
    }

    // Act & Assert - Remaining entries should be readable with correct content
    LOG_Entry entry;
    for (size_t i = entries_to_read; i < entries_written; i++) {
        char expected_message[LOG_MAX_MESSAGE_SIZE];
        snprintf(expected_message, sizeof(expected_message), "LOG_Entry %zu", i);

        TEST_ASSERT_TRUE(LOG_read_entry(&entry));
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
        LOG_read_entry(&entry);
    }

    // Act - Write new numbered entries after wraparound
    size_t new_entries_written = 0;
    while (LOG_write(LOG_LEVEL_INFO, "New LOG_Entry %zu", new_entries_written) > 0) {
        new_entries_written++;
    }

    // Skip remaining pre-wraparound entries
    LOG_Entry entry;
    while (LOG_read_entry(&entry)) {
        if (strstr(entry.message, "New LOG_Entry") != NULL) {
            // Found first new entry - it's now in the entry variable
            break;
        }
    }

    // Assert - New entries should be readable with correct content
    for (size_t i = 0; i < new_entries_written; i++) {
        char expected_message[LOG_MAX_MESSAGE_SIZE];
        snprintf(expected_message, sizeof(expected_message), "New LOG_Entry %zu", i);

        if (i == 0) {
            // First entry was already read above
            TEST_ASSERT_EQUAL_STRING(expected_message, entry.message);
        } else {
            TEST_ASSERT_TRUE(LOG_read_entry(&entry));
            TEST_ASSERT_EQUAL_STRING(expected_message, entry.message);
        }
    }
}

void test_LOG_consistent_state_on_multiple_initializations(void)
{
    // Arrange - Write a message before reinitialization
    char const *test_message = "Consistent state test";
    int bytes_written = LOG_write(LOG_LEVEL_INFO, test_message);
    TEST_ASSERT_GREATER_THAN(0, bytes_written);

    // Act - Reinitialize logging system
    LOG_Handle *handle_reinit = LOG_init();

    // Assert - Should return the same handle, state should be consistent
    TEST_ASSERT_EQUAL_PTR(g_log_handle, handle_reinit);
    TEST_ASSERT_TRUE(handle_reinit->initialized);
    TEST_ASSERT_EQUAL(bytes_written, LOG_available());

    // Verify we can still read the previously written message
    LOG_Entry entry;
    TEST_ASSERT_TRUE(LOG_read_entry(&entry));
    TEST_ASSERT_EQUAL(LOG_LEVEL_INFO, entry.level);
    TEST_ASSERT_EQUAL_STRING(test_message, entry.message);
}
