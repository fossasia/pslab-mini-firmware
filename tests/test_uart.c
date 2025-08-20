#include "unity.h"
#include "syscalls_config.h"
#include "mock_uart_ll.h"
#include "uart.h"
#include "error.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

// Test fixtures
static CircularBuffer g_rx_buffer;
static CircularBuffer g_tx_buffer;
static uint8_t g_rx_data[256];
static uint8_t g_tx_data[256];
static UART_Handle *g_test_handle;

// This flag is normally set by syscalls.c when claiming the UART bus
bool g_SYSCALLS_uart_claim = false;

void setUp(void)
{
    // Initialize buffers for each test
    circular_buffer_init(&g_rx_buffer, g_rx_data, sizeof(g_rx_data));
    circular_buffer_init(&g_tx_buffer, g_tx_data, sizeof(g_tx_data));

    // Initialize mocks
    mock_uart_ll_Init();
}

void tearDown(void)
{
    // Clean up UART handle if it exists
    if (g_test_handle != nullptr) {
        // Set up expectations for deinit - use Ignore to be flexible
        UART_LL_deinit_Ignore();
        UART_LL_set_idle_callback_Ignore();
        UART_LL_set_rx_complete_callback_Ignore();
        UART_LL_set_tx_complete_callback_Ignore();

        UART_deinit(g_test_handle);
        g_test_handle = nullptr;
    }

    // Clean up mocks after each test
    // Note: mock_uart_ll_Destroy() automatically verifies all expectations
    mock_uart_ll_Destroy();
}

void test_UART_init_success(void)
{
    // Arrange
    size_t bus = 0;

    // Set expectations for UART_LL calls - use Ignore for callbacks to avoid linking issues
    UART_LL_init_Expect(UART_BUS_0, g_rx_data, sizeof(g_rx_data));
    UART_LL_set_idle_callback_Ignore();
    UART_LL_set_rx_complete_callback_Ignore();
    UART_LL_set_tx_complete_callback_Ignore();

    // Act
    g_test_handle = UART_init(bus, &g_rx_buffer, &g_tx_buffer);

    // Assert
    TEST_ASSERT_NOT_NULL(g_test_handle);
}

void test_UART_init_null_rx_buffer(void)
{
    // Arrange
    size_t bus = 0;
    UART_Handle *handle = nullptr;
    Error caught_error = ERROR_NONE;

    // Act - pass nullptr rx_buffer
    TRY {
        handle = UART_init(bus, nullptr, &g_tx_buffer);
    } CATCH(caught_error) {
        // Expected to catch an error
    }

    // Assert
    TEST_ASSERT_EQUAL(ERROR_INVALID_ARGUMENT, caught_error);
    TEST_ASSERT_NULL(handle);
}

void test_UART_init_null_tx_buffer(void)
{
    // Arrange
    size_t bus = 0;
    UART_Handle *handle = nullptr;
    Error caught_error = ERROR_NONE;

    // Act - pass nullptr tx_buffer
    TRY {
        handle = UART_init(bus, &g_rx_buffer, nullptr);
    } CATCH(caught_error) {
        // Expected to catch an error
    }

    // Assert
    TEST_ASSERT_EQUAL(ERROR_INVALID_ARGUMENT, caught_error);
    TEST_ASSERT_NULL(handle);
}

void test_UART_init_invalid_bus(void)
{
    // Arrange
    size_t bus = UART_BUS_COUNT; // Invalid bus number
    UART_Handle *handle = nullptr;
    Error caught_error = ERROR_NONE;

    // Act
    TRY {
        handle = UART_init(bus, &g_rx_buffer, &g_tx_buffer);
    } CATCH(caught_error) {
        // Expected to catch an error
    }

    // Assert
    TEST_ASSERT_EQUAL(ERROR_INVALID_ARGUMENT, caught_error);
    TEST_ASSERT_NULL(handle);
}

void test_UART_init_syscalls_bus(void)
{
    // Arrange
    size_t bus = SYSCALLS_UART_BUS;
    UART_Handle *handle = nullptr;
    Error caught_error = ERROR_NONE;

    // Act
    TRY {
        handle = UART_init(bus, &g_rx_buffer, &g_tx_buffer);
    } CATCH(caught_error) {
        // Expected to catch an error since only syscalls can claim this bus
    }

    // Assert - Should fail since only syscalls can claim this bus
    TEST_ASSERT_EQUAL(ERROR_RESOURCE_UNAVAILABLE, caught_error);
    TEST_ASSERT_NULL(handle);
}

void test_UART_get_bus_count(void)
{
    // Act
    size_t count = UART_get_bus_count();

    // Assert
    TEST_ASSERT_EQUAL(UART_BUS_COUNT, count);
}

void test_UART_write_with_valid_handle(void)
{
    // Arrange
    size_t bus = 0;
    uint8_t test_data[] = {0x01, 0x02, 0x03, 0x04};

    // Set up expectations for init
    UART_LL_init_Expect(UART_BUS_0, g_rx_data, sizeof(g_rx_data));
    UART_LL_set_idle_callback_Ignore();
    UART_LL_set_rx_complete_callback_Ignore();
    UART_LL_set_tx_complete_callback_Ignore();

    g_test_handle = UART_init(bus, &g_rx_buffer, &g_tx_buffer);

    // Assert that init succeeded
    TEST_ASSERT_NOT_NULL(g_test_handle);

    // Expect the TX functions to be called when data is available
    UART_LL_tx_busy_ExpectAndReturn(UART_BUS_0, false);
    UART_LL_start_dma_tx_Expect(UART_BUS_0, test_data, sizeof(test_data));

    // Act
    uint32_t bytes_written = UART_write(g_test_handle, test_data, sizeof(test_data));

    // Assert - Initially buffer should be able to accept data
    TEST_ASSERT_GREATER_OR_EQUAL(4, bytes_written);
    TEST_ASSERT_LESS_OR_EQUAL(sizeof(test_data), bytes_written);
}

void test_UART_write_with_full_buffer(void)
{
    // Arrange
    size_t bus = 0;
    uint8_t test_data[] = {0x01, 0x02, 0x03, 0x04};

    // Set up expectations for init
    UART_LL_init_Expect(UART_BUS_0, g_rx_data, sizeof(g_rx_data));
    UART_LL_set_idle_callback_Ignore();
    UART_LL_set_rx_complete_callback_Ignore();
    UART_LL_set_tx_complete_callback_Ignore();

    g_test_handle = UART_init(bus, &g_rx_buffer, &g_tx_buffer);

    // Assert that init succeeded
    TEST_ASSERT_NOT_NULL(g_test_handle);

    // Advance TX buffer head to simulate a full buffer
    g_tx_buffer.head = g_tx_buffer.size - 1;

    // Simulate TX busy state
    UART_LL_tx_busy_ExpectAndReturn(UART_BUS_0, true);

    // Act
    uint32_t bytes_written = UART_write(g_test_handle, test_data, sizeof(test_data));

    // Assert - Should return 0 since buffer is full
    TEST_ASSERT_EQUAL(0, bytes_written);
}

void test_UART_write_with_nearly_full_buffer(void)
{
    // Arrange
    size_t bus = 0;
    uint8_t test_data[] = {0x01, 0x02, 0x03, 0x04};
    uint8_t fill_data[252]; // Fill buffer to near capacity: 255 - 3 = 252 bytes

    // Set up expectations for init
    UART_LL_init_Expect(UART_BUS_0, g_rx_data, sizeof(g_rx_data));
    UART_LL_set_idle_callback_Ignore();
    UART_LL_set_rx_complete_callback_Ignore();
    UART_LL_set_tx_complete_callback_Ignore();

    g_test_handle = UART_init(bus, &g_rx_buffer, &g_tx_buffer);

    // Assert that init succeeded
    TEST_ASSERT_NOT_NULL(g_test_handle);

    // Fill the buffer to near capacity (252 bytes), leaving room for only 3 more bytes
    memset(fill_data, 0xAA, sizeof(fill_data));
    uint32_t fill_written = circular_buffer_write(&g_tx_buffer, fill_data, sizeof(fill_data));
    TEST_ASSERT_EQUAL(sizeof(fill_data), fill_written);

    // Simulate TX not busy state
    UART_LL_tx_busy_ExpectAndReturn(UART_BUS_0, false);
    // When transmission starts, it should send all 255 bytes (252 + 3) that are in the buffer
    UART_LL_start_dma_tx_Expect(UART_BUS_0, g_tx_data, 255);

    // Act
    uint32_t bytes_written = UART_write(g_test_handle, test_data, sizeof(test_data));

    // Assert - Should write 3 bytes since that's all that fits
    TEST_ASSERT_EQUAL(3, bytes_written);
}

void test_UART_write_with_null_handle(void)
{
    // Arrange
    uint8_t test_data[] = {0x01, 0x02, 0x03, 0x04};

    // Act
    uint32_t bytes_written = UART_write(NULL, test_data, sizeof(test_data));

    // Assert
    TEST_ASSERT_EQUAL(0, bytes_written);
}

void test_UART_read_with_valid_handle(void)
{
    // Arrange
    size_t bus = 0;
    uint8_t read_buffer[10];

    // Set up expectations for init
    UART_LL_init_Expect(UART_BUS_0, g_rx_data, 256);
    UART_LL_set_idle_callback_Ignore();
    UART_LL_set_rx_complete_callback_Ignore();
    UART_LL_set_tx_complete_callback_Ignore();

    // Mock DMA position for read operation
    UART_LL_get_dma_position_ExpectAndReturn(UART_BUS_0, 0);

    g_test_handle = UART_init(bus, &g_rx_buffer, &g_tx_buffer);

    // Act
    uint32_t bytes_read = UART_read(g_test_handle, read_buffer, sizeof(read_buffer));

    // Assert
    TEST_ASSERT_EQUAL(0, bytes_read); // Buffer is empty initially
}

void test_UART_read_with_data_available(void)
{
    // Arrange
    size_t bus = 0;
    uint8_t read_buffer[10];
    uint8_t test_data[] = {0x01, 0x02, 0x03, 0x04};

    // Set up expectations for init
    UART_LL_init_Expect(UART_BUS_0, g_rx_data, 256);
    UART_LL_set_idle_callback_Ignore();
    UART_LL_set_rx_complete_callback_Ignore();
    UART_LL_set_tx_complete_callback_Ignore();

    // Mock DMA position to simulate data available
    UART_LL_get_dma_position_ExpectAndReturn(UART_BUS_0, sizeof(test_data));

    g_test_handle = UART_init(bus, &g_rx_buffer, &g_tx_buffer);

    // Simulate data in the RX buffer
    circular_buffer_write(&g_rx_buffer, test_data, sizeof(test_data));

    // Act
    uint32_t bytes_read = UART_read(g_test_handle, read_buffer, sizeof(read_buffer));

    // Assert - Should read the available data
    TEST_ASSERT_EQUAL(sizeof(test_data), bytes_read);
    TEST_ASSERT_EQUAL_MEMORY(test_data, read_buffer, bytes_read);
}

void test_UART_read_with_null_handle(void)
{
    // Arrange
    uint8_t read_buffer[10];

    // Act
    uint32_t bytes_read = UART_read(NULL, read_buffer, sizeof(read_buffer));

    // Assert
    TEST_ASSERT_EQUAL(0, bytes_read);
}

void test_UART_rx_ready_with_valid_handle(void)
{
    // Arrange
    size_t bus = 0;

    // Set up expectations for init
    UART_LL_init_Expect(UART_BUS_0, g_rx_data, 256);
    UART_LL_set_idle_callback_Ignore();
    UART_LL_set_rx_complete_callback_Ignore();
    UART_LL_set_tx_complete_callback_Ignore();

    // Mock DMA position for rx_ready check
    UART_LL_get_dma_position_ExpectAndReturn(UART_BUS_0, 0);

    g_test_handle = UART_init(bus, &g_rx_buffer, &g_tx_buffer);

    // Act
    bool ready = UART_rx_ready(g_test_handle);

    // Assert
    TEST_ASSERT_FALSE(ready); // Buffer is empty initially
}

void test_UART_deinit_with_valid_handle(void)
{
    // Arrange
    size_t bus = 0;

    // Set up expectations for init
    UART_LL_init_Expect(UART_BUS_0, g_rx_data, 256);
    UART_LL_set_idle_callback_Ignore();
    UART_LL_set_rx_complete_callback_Ignore();
    UART_LL_set_tx_complete_callback_Ignore();

    g_test_handle = UART_init(bus, &g_rx_buffer, &g_tx_buffer);

    // Set up expectations for deinit - these MUST be called
    UART_LL_deinit_Expect(UART_BUS_0);
    UART_LL_set_idle_callback_Expect(UART_BUS_0, nullptr);
    UART_LL_set_rx_complete_callback_Expect(UART_BUS_0, nullptr);
    UART_LL_set_tx_complete_callback_Expect(UART_BUS_0, nullptr);

    // Act
    UART_deinit(g_test_handle);
    g_test_handle = nullptr; // Manually set to nullptr since we're testing explicit deinit

    // Assert - Mock verification happens automatically in tearDown()
}

void test_UART_deinit_with_null_handle(void)
{
    // Act
    UART_deinit(nullptr);

    // Assert - No crash, no errors
    TEST_PASS();
}
