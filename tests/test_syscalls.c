#include "unity.h"
#include "mock_uart_ll.h"
#include "uart.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <reent.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

// Forward declarations of functions we'll test
struct _reent;
struct stat;
typedef long _ssize_t;

_ssize_t _read_r(struct _reent *r, int fd, void *buf, size_t cnt);
_ssize_t _write_r(struct _reent *r, int fd, void const *buf, size_t cnt);
int _fstat_r(struct _reent *r, int fd, struct stat *st);
int _isatty_r(struct _reent *r, int fd);
void syscalls_uart_deinit(void);

// Test fixtures
static struct _reent test_reent;

void setUp(void)
{
    // point the global errno‐macro into our local reent
    __impure_ptr = &test_reent;
    // Initialize the test reent structure
    test_reent._errno = 0;

    // Initialize mocks
    mock_uart_ll_Init();

    // Set up default ignores for all UART_LL functions to avoid argument validation issues
    UART_LL_init_Ignore();
    UART_LL_set_idle_callback_Ignore();
    UART_LL_set_rx_complete_callback_Ignore();
    UART_LL_set_tx_complete_callback_Ignore();
    UART_LL_deinit_Ignore();
}

void tearDown(void)
{
    // Deinitialize UART to reset state between tests
    syscalls_uart_deinit();

    // Clean up mocks
    mock_uart_ll_Destroy();
}

// Test _write_r function for stdout
void test_write_r_stdout_success(void)
{
    // Arrange
    char test_data[] = "Hello, World!";
    size_t data_len = strlen(test_data);

    // Set up expectations for write operation only
    UART_LL_tx_busy_ExpectAndReturn(UART_BUS_0, false);
    UART_LL_start_dma_tx_Expect(UART_BUS_0, (uint8_t*)test_data, data_len);

    // Act
    _ssize_t result = _write_r(&test_reent, STDOUT_FILENO, test_data, data_len);

    // Assert
    TEST_ASSERT_EQUAL(data_len, result);
    TEST_ASSERT_EQUAL(0, test_reent._errno);
}

// Test _write_r function for stderr
void test_write_r_stderr_success(void)
{
    // Arrange
    char test_data[] = "Error message";
    size_t data_len = strlen(test_data);

    // Set up expectations for write operation only
    UART_LL_tx_busy_ExpectAndReturn(UART_BUS_0, false);
    UART_LL_start_dma_tx_Expect(UART_BUS_0, (uint8_t*)test_data, data_len);

    // Act
    _ssize_t result = _write_r(&test_reent, STDERR_FILENO, test_data, data_len);

    // Assert
    TEST_ASSERT_EQUAL(data_len, result);
    TEST_ASSERT_EQUAL(0, test_reent._errno);
}

// Test _write_r function with invalid file descriptor
void test_write_r_invalid_fd(void)
{
    // Arrange
    char test_data[] = "Test data";
    size_t data_len = strlen(test_data);
    int invalid_fd = 5; // Not stdout or stderr

    // Act
    _ssize_t result = _write_r(&test_reent, invalid_fd, test_data, data_len);

    // Assert
    TEST_ASSERT_EQUAL(-1, result);
    TEST_ASSERT_EQUAL(EBADF, test_reent._errno);
}

void test_read_r_null_buffer(void)
{
    // Arrange
    size_t buffer_size = 32;

    // Act
    _ssize_t result = _read_r(&test_reent, STDIN_FILENO, nullptr, buffer_size);

    // Assert
    TEST_ASSERT_EQUAL(-1, result);
    TEST_ASSERT_EQUAL(EFAULT, test_reent._errno); // Null buffer should return EFAULT
}

// Test _read_r function with zero-length read (should succeed)
void test_read_r_zero_length(void)
{
    // Arrange
    uint8_t read_buffer[32];

    // Act
    _ssize_t result = _read_r(&test_reent, STDIN_FILENO, read_buffer, 0);

    // Assert
    TEST_ASSERT_EQUAL(0, result); // Zero-length reads should always succeed
    TEST_ASSERT_EQUAL(0, test_reent._errno);
}

// Test _read_r function for stdin (should fail - reads not supported)
void test_read_r_stdin_not_supported(void)
{
    // Arrange
    uint8_t read_buffer[32];
    size_t buffer_size = sizeof(read_buffer);

    // Act
    _ssize_t result = _read_r(&test_reent, STDIN_FILENO, read_buffer, buffer_size);

    // Assert
    TEST_ASSERT_EQUAL(-1, result); // Should fail
    TEST_ASSERT_EQUAL(ENOSYS, test_reent._errno); // Function not implemented
}

// Test _read_r function with invalid file descriptor
void test_read_r_invalid_fd(void)
{
    // Arrange
    uint8_t read_buffer[32];
    size_t buffer_size = sizeof(read_buffer);
    int invalid_fd = 5; // Not stdin

    // Act
    _ssize_t result = _read_r(&test_reent, invalid_fd, read_buffer, buffer_size);

    // Assert
    TEST_ASSERT_EQUAL(-1, result);
    TEST_ASSERT_EQUAL(ENOSYS, test_reent._errno); // All reads not supported
}

// Test with TX buffer full scenario
void test_write_r_tx_buffer_full(void)
{
    // Arrange
    char test_data[] = "Test data";
    size_t data_len = strlen(test_data);

    // Set up expectations for UART initialization
    UART_LL_init_Ignore();
    UART_LL_set_idle_callback_Ignore();
    UART_LL_set_rx_complete_callback_Ignore();
    UART_LL_set_tx_complete_callback_Ignore();

    // Simulate TX not busy initially, so transmission can start
    UART_LL_tx_busy_ExpectAndReturn(UART_BUS_0, false);

    // First, fill the TX buffer with data (255 bytes to leave 1 slot free)
    // The buffer size is 256, and full condition is when (head + 1) % size == tail
    // So we can write 255 bytes before it becomes full
    char filler_data[255];
    memset(filler_data, 'X', sizeof(filler_data));

    // Expect start_dma_tx to be called for the initial filler data
    UART_LL_start_dma_tx_Expect(UART_BUS_0, (uint8_t*)filler_data, 255);

    // Write filler data to fill the buffer
    _ssize_t filler_result = _write_r(&test_reent, 1, filler_data, 255);
    TEST_ASSERT_EQUAL(255, filler_result); // Should write all filler data

    // Now try to write more data - this should fail or write less
    // TX should still be busy from the previous operation
    UART_LL_tx_busy_ExpectAndReturn(UART_BUS_0, true);

    // Act - try to write when buffer is full
    _ssize_t result = _write_r(&test_reent, 1, test_data, data_len);

    // Assert - should return -1 and set EAGAIN since buffer is full
    TEST_ASSERT_EQUAL(-1, result);
    TEST_ASSERT_EQUAL(EAGAIN, test_reent._errno);
}

// Test multiple consecutive writes
void test_multiple_writes(void)
{
    // Arrange
    char test_data1[] = "First ";
    char test_data2[] = "Second";

    // Set up expectations for UART initialization (only once)
    UART_LL_init_Ignore();
    UART_LL_set_idle_callback_Ignore();
    UART_LL_set_rx_complete_callback_Ignore();
    UART_LL_set_tx_complete_callback_Ignore();

    // Set up expectations for first write
    UART_LL_tx_busy_ExpectAndReturn(UART_BUS_0, false);
    UART_LL_start_dma_tx_Expect(UART_BUS_0, (uint8_t*)test_data1, 6);

    // Act - first write
    _ssize_t result1 = _write_r(&test_reent, STDOUT_FILENO, test_data1, 6);

    // For the second write, TX is now busy from the first write
    // So the data will be queued but no new DMA transfer will start
    UART_LL_tx_busy_ExpectAndReturn(UART_BUS_0, true);

    // Act - second write (should queue data but not start new transmission)
    _ssize_t result2 = _write_r(&test_reent, STDOUT_FILENO, test_data2, 6);

    // Assert
    TEST_ASSERT_EQUAL(6, result1);
    TEST_ASSERT_EQUAL(6, result2); // Should still return 6 (data was queued)
    TEST_ASSERT_EQUAL(0, test_reent._errno);
}

// Test _fstat_r function for stdin (should fail - reads not supported)
void test_fstat_r_stdin_not_supported(void)
{
    // Arrange
    struct stat st;

    // Act
    int result = _fstat_r(&test_reent, STDIN_FILENO, &st);

    // Assert
    TEST_ASSERT_EQUAL(-1, result); // Should fail
    TEST_ASSERT_EQUAL(EBADF, test_reent._errno); // Bad file descriptor
}

// Test _fstat_r function for stdout
void test_fstat_r_stdout_success(void)
{
    // Arrange
    struct stat st;

    // Act
    int result = _fstat_r(&test_reent, STDOUT_FILENO, &st);

    // Assert
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_EQUAL(S_IFCHR, st.st_mode); // Should be character device
    TEST_ASSERT_EQUAL(0, st.st_size);
    TEST_ASSERT_EQUAL(0, test_reent._errno);
}

// Test _fstat_r function for stderr
void test_fstat_r_stderr_success(void)
{
    // Arrange
    struct stat st;

    // Act
    int result = _fstat_r(&test_reent, STDERR_FILENO, &st);

    // Assert
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_EQUAL(S_IFCHR, st.st_mode); // Should be character device
    TEST_ASSERT_EQUAL(0, st.st_size);
    TEST_ASSERT_EQUAL(0, test_reent._errno);
}

// Test _fstat_r function with invalid file descriptor
void test_fstat_r_invalid_fd(void)
{
    // Arrange
    struct stat st;
    int invalid_fd = 5; // Not stdin, stdout, or stderr

    // Act
    int result = _fstat_r(&test_reent, invalid_fd, &st);

    // Assert
    TEST_ASSERT_EQUAL(-1, result);
    TEST_ASSERT_EQUAL(EBADF, test_reent._errno);
}

// Test _fstat_r function with null stat pointer
void test_fstat_r_null_stat(void)
{
    // Act
    int result = _fstat_r(&test_reent, STDIN_FILENO, nullptr);

    // Assert
    TEST_ASSERT_EQUAL(-1, result);
    TEST_ASSERT_EQUAL(EFAULT, test_reent._errno);
}

// Test _isatty_r function for stdin (should fail - reads not supported)
void test_isatty_r_stdin_not_tty(void)
{
    // Act
    int result = _isatty_r(&test_reent, STDIN_FILENO);

    // Assert
    TEST_ASSERT_EQUAL(0, result); // Should return 0 (false)
    TEST_ASSERT_EQUAL(ENOTTY, test_reent._errno);
}

// Test _isatty_r function for stdout
void test_isatty_r_stdout_is_tty(void)
{
    // Act
    int result = _isatty_r(&test_reent, STDOUT_FILENO);

    // Assert
    TEST_ASSERT_EQUAL(1, result); // Should return 1 (true)
    TEST_ASSERT_EQUAL(0, test_reent._errno);
}

// Test _isatty_r function for stderr
void test_isatty_r_stderr_is_tty(void)
{
    // Act
    int result = _isatty_r(&test_reent, STDERR_FILENO);

    // Assert
    TEST_ASSERT_EQUAL(1, result); // Should return 1 (true)
    TEST_ASSERT_EQUAL(0, test_reent._errno);
}

// Test _isatty_r function with invalid file descriptor
void test_isatty_r_invalid_fd_not_tty(void)
{
    // Arrange
    int invalid_fd = 5; // Not stdin, stdout, or stderr

    // Act
    int result = _isatty_r(&test_reent, invalid_fd);

    // Assert
    TEST_ASSERT_EQUAL(0, result); // Should return 0 (false)
    TEST_ASSERT_EQUAL(ENOTTY, test_reent._errno);
}
