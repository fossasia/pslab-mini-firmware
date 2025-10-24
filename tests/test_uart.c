#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "unity.h"
#include "mock_uart_ll.h"
#include "mock_platform.h"

#include "util/error.h"

#include "uart.h"


// Test fixtures
static CircularBuffer g_rx_buffer;
static CircularBuffer g_tx_buffer;
static uint8_t g_rx_data[256];
static uint8_t g_tx_data[256];
static UART_Handle *g_test_handle;

void setUp(void)
{
    // Initialize buffers for each test
    circular_buffer_init(&g_rx_buffer, g_rx_data, sizeof(g_rx_data));
    circular_buffer_init(&g_tx_buffer, g_tx_data, sizeof(g_tx_data));

    // Initialize mocks
    mock_uart_ll_Init();
    mock_platform_Init();
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
    mock_platform_Destroy();
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

void test_UART_flush_with_valid_handle_no_timeout(void)
{
    // Arrange
    size_t bus = 0;

    // Set up expectations for init
    UART_LL_init_Expect(UART_BUS_0, g_rx_data, sizeof(g_rx_data));
    UART_LL_set_idle_callback_Ignore();
    UART_LL_set_rx_complete_callback_Ignore();
    UART_LL_set_tx_complete_callback_Ignore();

    g_test_handle = UART_init(bus, &g_rx_buffer, &g_tx_buffer);

    // Setup expectation - TX is not busy so transmission completes immediately
    UART_LL_tx_busy_ExpectAndReturn(UART_BUS_0, false);
    PLATFORM_get_tick_ExpectAndReturn(100);

    // Act - flush with no timeout (0 means wait indefinitely)
    bool result = UART_flush(g_test_handle, 0);

    // Assert - Should succeed since buffer is empty
    TEST_ASSERT_TRUE(result);
}

void test_UART_flush_with_timeout_success(void)
{
    // Arrange
    size_t bus = 0;

    // Set up expectations for init
    UART_LL_init_Expect(UART_BUS_0, g_rx_data, sizeof(g_rx_data));
    UART_LL_set_idle_callback_Ignore();
    UART_LL_set_rx_complete_callback_Ignore();
    UART_LL_set_tx_complete_callback_Ignore();

    g_test_handle = UART_init(bus, &g_rx_buffer, &g_tx_buffer);

    // Setup expectation - TX is not busy so no transmission needed
    UART_LL_tx_busy_ExpectAndReturn(UART_BUS_0, false);
    PLATFORM_get_tick_ExpectAndReturn(1000);

    // Act - flush with timeout (should succeed immediately since buffer is empty)
    bool result = UART_flush(g_test_handle, 100); // 100ms timeout

    // Assert - Should succeed since buffer is empty
    TEST_ASSERT_TRUE(result);
}

void test_UART_flush_with_timeout_failure(void)
{
    // Arrange
    size_t bus = 0;
    uint32_t current_time = 1000;
    uint8_t test_data[] = {0x01, 0x02, 0x03, 0x04};

    // Set up expectations for init
    UART_LL_init_Expect(UART_BUS_0, g_rx_data, sizeof(g_rx_data));
    UART_LL_set_idle_callback_Ignore();
    UART_LL_set_rx_complete_callback_Ignore();
    UART_LL_set_tx_complete_callback_Ignore();

    g_test_handle = UART_init(bus, &g_rx_buffer, &g_tx_buffer);

    // Add some data to the TX buffer
    uint32_t bytes_written = circular_buffer_write(&g_tx_buffer, test_data, sizeof(test_data));
    TEST_ASSERT_EQUAL(sizeof(test_data), bytes_written);

    // Setup expectations - TX starts but never completes
    UART_LL_tx_busy_ExpectAndReturn(UART_BUS_0, false);
    UART_LL_start_dma_tx_Expect(UART_BUS_0, test_data, sizeof(test_data));

    // Mock time calls - start time, then time that exceeds timeout
    PLATFORM_get_tick_ExpectAndReturn(current_time);  // start_time
    PLATFORM_get_tick_ExpectAndReturn(current_time + 150);  // elapsed check - exceeds timeout

    // Act
    bool result = UART_flush(g_test_handle, 100); // 100ms timeout

    // Assert - Should fail due to timeout
    TEST_ASSERT_FALSE(result);
}

void test_UART_flush_with_null_handle(void)
{
    // Act
    bool result = UART_flush(NULL, 100);

    // Assert
    TEST_ASSERT_FALSE(result);
}

// ============================================================================
// UART Passthrough Tests
// ============================================================================

void test_UART_enable_passthrough_success(void)
{
    // Arrange - Initialize two UART buses with separate buffers
    CircularBuffer rx_buffer1, tx_buffer1;
    CircularBuffer rx_buffer2, tx_buffer2;
    uint8_t rx_data1[256], tx_data1[256];
    uint8_t rx_data2[256], tx_data2[256];

    circular_buffer_init(&rx_buffer1, rx_data1, sizeof(rx_data1));
    circular_buffer_init(&tx_buffer1, tx_data1, sizeof(tx_data1));
    circular_buffer_init(&rx_buffer2, rx_data2, sizeof(rx_data2));
    circular_buffer_init(&tx_buffer2, tx_data2, sizeof(tx_data2));

    // Set up expectations for init of handle1
    UART_LL_init_Expect(UART_BUS_0, rx_data1, sizeof(rx_data1));
    UART_LL_set_idle_callback_Ignore();
    UART_LL_set_rx_complete_callback_Ignore();
    UART_LL_set_tx_complete_callback_Ignore();

    UART_Handle *handle1 = UART_init(0, &rx_buffer1, &tx_buffer1);
    TEST_ASSERT_NOT_NULL(handle1);

    // Set up expectations for init of handle2
    UART_LL_init_Expect(UART_BUS_1, rx_data2, sizeof(rx_data2));
    UART_LL_set_idle_callback_Ignore();
    UART_LL_set_rx_complete_callback_Ignore();
    UART_LL_set_tx_complete_callback_Ignore();

    UART_Handle *handle2 = UART_init(1, &rx_buffer2, &tx_buffer2);
    TEST_ASSERT_NOT_NULL(handle2);

    // Mock DMA position calls during passthrough enable (called by UART_set_rx_callback)
    UART_LL_get_dma_position_ExpectAndReturn(UART_BUS_0, 0);
    UART_LL_get_dma_position_ExpectAndReturn(UART_BUS_1, 0);

    // Act - Enable passthrough
    UART_enable_passthrough(handle1, handle2);

    // Assert - Passthrough enabled, no errors thrown (test passes if no exception)

    // Cleanup
    UART_disable_passthrough(handle1, handle2);

    UART_LL_deinit_Ignore();
    UART_LL_set_idle_callback_Ignore();
    UART_LL_set_rx_complete_callback_Ignore();
    UART_LL_set_tx_complete_callback_Ignore();

    UART_deinit(handle1);
    UART_deinit(handle2);
}

void test_UART_enable_passthrough_null_handles(void)
{
    // Arrange
    Error caught_error = ERROR_NONE;

    // Act - Try to enable passthrough with null handles
    TRY {
        UART_enable_passthrough(nullptr, nullptr);
    } CATCH(caught_error) {
        // Expected to catch an error
    }

    // Assert
    TEST_ASSERT_EQUAL(ERROR_DEVICE_NOT_READY, caught_error);
}

void test_UART_enable_passthrough_one_null_handle(void)
{
    // Arrange
    CircularBuffer rx_buffer1, tx_buffer1;
    uint8_t rx_data1[256], tx_data1[256];

    circular_buffer_init(&rx_buffer1, rx_data1, sizeof(rx_data1));
    circular_buffer_init(&tx_buffer1, tx_data1, sizeof(tx_data1));

    UART_LL_init_Expect(UART_BUS_0, rx_data1, sizeof(rx_data1));
    UART_LL_set_idle_callback_Ignore();
    UART_LL_set_rx_complete_callback_Ignore();
    UART_LL_set_tx_complete_callback_Ignore();

    UART_Handle *handle1 = UART_init(0, &rx_buffer1, &tx_buffer1);
    TEST_ASSERT_NOT_NULL(handle1);

    Error caught_error = ERROR_NONE;

    // Act - Try to enable passthrough with one null handle
    TRY {
        UART_enable_passthrough(handle1, nullptr);
    } CATCH(caught_error) {
        // Expected to catch an error
    }

    // Assert
    TEST_ASSERT_EQUAL(ERROR_DEVICE_NOT_READY, caught_error);

    // Cleanup
    UART_LL_deinit_Ignore();
    UART_LL_set_idle_callback_Ignore();
    UART_LL_set_rx_complete_callback_Ignore();
    UART_LL_set_tx_complete_callback_Ignore();

    UART_deinit(handle1);
}

void test_UART_passthrough_data_flow_bus0_to_bus1(void)
{
    // Arrange - Initialize two UART buses
    CircularBuffer rx_buffer1, tx_buffer1;
    CircularBuffer rx_buffer2, tx_buffer2;
    uint8_t rx_data1[256], tx_data1[256];
    uint8_t rx_data2[256], tx_data2[256];

    circular_buffer_init(&rx_buffer1, rx_data1, sizeof(rx_data1));
    circular_buffer_init(&tx_buffer1, tx_data1, sizeof(tx_data1));
    circular_buffer_init(&rx_buffer2, rx_data2, sizeof(rx_data2));
    circular_buffer_init(&tx_buffer2, tx_data2, sizeof(tx_data2));

    // Set up expectations for init
    UART_LL_init_Expect(UART_BUS_0, rx_data1, sizeof(rx_data1));
    UART_LL_set_idle_callback_Ignore();
    UART_LL_set_rx_complete_callback_Ignore();
    UART_LL_set_tx_complete_callback_Ignore();

    UART_Handle *handle1 = UART_init(0, &rx_buffer1, &tx_buffer1);

    UART_LL_init_Expect(UART_BUS_1, rx_data2, sizeof(rx_data2));
    UART_LL_set_idle_callback_Ignore();
    UART_LL_set_rx_complete_callback_Ignore();
    UART_LL_set_tx_complete_callback_Ignore();

    UART_Handle *handle2 = UART_init(1, &rx_buffer2, &tx_buffer2);

    // Mock DMA position calls during passthrough enable
    UART_LL_get_dma_position_ExpectAndReturn(UART_BUS_0, 0);
    UART_LL_get_dma_position_ExpectAndReturn(UART_BUS_1, 0);

    // Enable passthrough
    UART_enable_passthrough(handle1, handle2);

    // Simulate data received on bus 0
    uint8_t test_data[] = {0xAA, 0xBB, 0xCC, 0xDD};
    uint32_t written = circular_buffer_write(handle1->rx_buffer, test_data, sizeof(test_data));
    TEST_ASSERT_EQUAL(sizeof(test_data), written);

    // Update buffer head to simulate DMA
    handle1->rx_buffer->head = sizeof(test_data);

    // Mock DMA position for callback triggering
    UART_LL_get_dma_position_ExpectAndReturn(UART_BUS_0, sizeof(test_data));

    // Expect transmission to be started on bus 1 (not busy)
    UART_LL_tx_busy_ExpectAndReturn(UART_BUS_1, false);
    UART_LL_start_dma_tx_Expect(UART_BUS_1, test_data, sizeof(test_data));

    // Act - Check RX available which triggers callback
    uint32_t available = UART_rx_available(handle1);

    // Assert - Data should be available
    TEST_ASSERT_EQUAL(sizeof(test_data), available);

    // Verify data is in the correct buffer (bus1's TX buffer points to bus0's RX buffer in passthrough)
    uint8_t read_buffer[10];
    uint32_t bytes_read = circular_buffer_read(handle2->tx_buffer, read_buffer, sizeof(read_buffer));
    TEST_ASSERT_EQUAL(sizeof(test_data), bytes_read);
    TEST_ASSERT_EQUAL_MEMORY(test_data, read_buffer, sizeof(test_data));

    // Cleanup
    UART_disable_passthrough(handle1, handle2);

    UART_LL_deinit_Ignore();
    UART_LL_set_idle_callback_Ignore();
    UART_LL_set_rx_complete_callback_Ignore();
    UART_LL_set_tx_complete_callback_Ignore();

    UART_deinit(handle1);
    UART_deinit(handle2);
}

void test_UART_passthrough_data_flow_bus1_to_bus0(void)
{
    // Arrange - Initialize two UART buses
    CircularBuffer rx_buffer1, tx_buffer1;
    CircularBuffer rx_buffer2, tx_buffer2;
    uint8_t rx_data1[256], tx_data1[256];
    uint8_t rx_data2[256], tx_data2[256];

    circular_buffer_init(&rx_buffer1, rx_data1, sizeof(rx_data1));
    circular_buffer_init(&tx_buffer1, tx_data1, sizeof(tx_data1));
    circular_buffer_init(&rx_buffer2, rx_data2, sizeof(rx_data2));
    circular_buffer_init(&tx_buffer2, tx_data2, sizeof(tx_data2));

    // Set up expectations for init
    UART_LL_init_Expect(UART_BUS_0, rx_data1, sizeof(rx_data1));
    UART_LL_set_idle_callback_Ignore();
    UART_LL_set_rx_complete_callback_Ignore();
    UART_LL_set_tx_complete_callback_Ignore();

    UART_Handle *handle1 = UART_init(0, &rx_buffer1, &tx_buffer1);

    UART_LL_init_Expect(UART_BUS_1, rx_data2, sizeof(rx_data2));
    UART_LL_set_idle_callback_Ignore();
    UART_LL_set_rx_complete_callback_Ignore();
    UART_LL_set_tx_complete_callback_Ignore();

    UART_Handle *handle2 = UART_init(1, &rx_buffer2, &tx_buffer2);

    // Mock DMA position calls during passthrough enable
    UART_LL_get_dma_position_ExpectAndReturn(UART_BUS_0, 0);
    UART_LL_get_dma_position_ExpectAndReturn(UART_BUS_1, 0);

    // Enable passthrough
    UART_enable_passthrough(handle1, handle2);

    // Simulate data received on bus 1
    uint8_t test_data[] = {0x11, 0x22, 0x33, 0x44, 0x55};
    uint32_t written = circular_buffer_write(handle2->rx_buffer, test_data, sizeof(test_data));
    TEST_ASSERT_EQUAL(sizeof(test_data), written);

    // Update buffer head to simulate DMA
    handle2->rx_buffer->head = sizeof(test_data);

    // Mock DMA position for callback triggering
    UART_LL_get_dma_position_ExpectAndReturn(UART_BUS_1, sizeof(test_data));

    // Expect transmission to be started on bus 0 (not busy)
    UART_LL_tx_busy_ExpectAndReturn(UART_BUS_0, false);
    UART_LL_start_dma_tx_Expect(UART_BUS_0, test_data, sizeof(test_data));

    // Act - Check RX available which triggers callback
    uint32_t available = UART_rx_available(handle2);

    // Assert - Data should be available
    TEST_ASSERT_EQUAL(sizeof(test_data), available);

    // Verify data is in the correct buffer (bus0's TX buffer points to bus1's RX buffer in passthrough)
    uint8_t read_buffer[10];
    uint32_t bytes_read = circular_buffer_read(handle1->tx_buffer, read_buffer, sizeof(read_buffer));
    TEST_ASSERT_EQUAL(sizeof(test_data), bytes_read);
    TEST_ASSERT_EQUAL_MEMORY(test_data, read_buffer, sizeof(test_data));

    // Cleanup
    UART_disable_passthrough(handle1, handle2);

    UART_LL_deinit_Ignore();
    UART_LL_set_idle_callback_Ignore();
    UART_LL_set_rx_complete_callback_Ignore();
    UART_LL_set_tx_complete_callback_Ignore();

    UART_deinit(handle1);
    UART_deinit(handle2);
}

void test_UART_passthrough_bidirectional_data_flow(void)
{
    // Arrange - Initialize two UART buses
    CircularBuffer rx_buffer1, tx_buffer1;
    CircularBuffer rx_buffer2, tx_buffer2;
    uint8_t rx_data1[256], tx_data1[256];
    uint8_t rx_data2[256], tx_data2[256];

    circular_buffer_init(&rx_buffer1, rx_data1, sizeof(rx_data1));
    circular_buffer_init(&tx_buffer1, tx_data1, sizeof(tx_data1));
    circular_buffer_init(&rx_buffer2, rx_data2, sizeof(rx_data2));
    circular_buffer_init(&tx_buffer2, tx_data2, sizeof(tx_data2));

    // Set up expectations for init
    UART_LL_init_Expect(UART_BUS_0, rx_data1, sizeof(rx_data1));
    UART_LL_set_idle_callback_Ignore();
    UART_LL_set_rx_complete_callback_Ignore();
    UART_LL_set_tx_complete_callback_Ignore();

    UART_Handle *handle1 = UART_init(0, &rx_buffer1, &tx_buffer1);

    UART_LL_init_Expect(UART_BUS_1, rx_data2, sizeof(rx_data2));
    UART_LL_set_idle_callback_Ignore();
    UART_LL_set_rx_complete_callback_Ignore();
    UART_LL_set_tx_complete_callback_Ignore();

    UART_Handle *handle2 = UART_init(1, &rx_buffer2, &tx_buffer2);

    // Mock DMA position calls during passthrough enable
    UART_LL_get_dma_position_ExpectAndReturn(UART_BUS_0, 0);
    UART_LL_get_dma_position_ExpectAndReturn(UART_BUS_1, 0);

    // Enable passthrough
    UART_enable_passthrough(handle1, handle2);

    // Simulate data received on both buses
    uint8_t test_data1[] = {0xAA, 0xBB, 0xCC};
    uint8_t test_data2[] = {0x11, 0x22, 0x33, 0x44};

    circular_buffer_write(handle1->rx_buffer, test_data1, sizeof(test_data1));
    handle1->rx_buffer->head = sizeof(test_data1);

    circular_buffer_write(handle2->rx_buffer, test_data2, sizeof(test_data2));
    handle2->rx_buffer->head = sizeof(test_data2);

    // Mock DMA positions and transmissions for bus 0 -> bus 1
    UART_LL_get_dma_position_ExpectAndReturn(UART_BUS_0, sizeof(test_data1));
    UART_LL_tx_busy_ExpectAndReturn(UART_BUS_1, false);
    UART_LL_start_dma_tx_Expect(UART_BUS_1, test_data1, sizeof(test_data1));

    // Act - Check RX on bus 0
    uint32_t available1 = UART_rx_available(handle1);

    // Mock DMA positions and transmissions for bus 1 -> bus 0
    UART_LL_get_dma_position_ExpectAndReturn(UART_BUS_1, sizeof(test_data2));
    UART_LL_tx_busy_ExpectAndReturn(UART_BUS_0, false);
    UART_LL_start_dma_tx_Expect(UART_BUS_0, test_data2, sizeof(test_data2));

    // Act - Check RX on bus 1
    uint32_t available2 = UART_rx_available(handle2);

    // Assert - Data should be available on both buses
    TEST_ASSERT_EQUAL(sizeof(test_data1), available1);
    TEST_ASSERT_EQUAL(sizeof(test_data2), available2);

    // Cleanup
    UART_disable_passthrough(handle1, handle2);

    UART_LL_deinit_Ignore();
    UART_LL_set_idle_callback_Ignore();
    UART_LL_set_rx_complete_callback_Ignore();
    UART_LL_set_tx_complete_callback_Ignore();

    UART_deinit(handle1);
    UART_deinit(handle2);
}

void test_UART_disable_passthrough_success(void)
{
    // Arrange - Initialize two UART buses
    CircularBuffer rx_buffer1, tx_buffer1;
    CircularBuffer rx_buffer2, tx_buffer2;
    uint8_t rx_data1[256], tx_data1[256];
    uint8_t rx_data2[256], tx_data2[256];

    circular_buffer_init(&rx_buffer1, rx_data1, sizeof(rx_data1));
    circular_buffer_init(&tx_buffer1, tx_data1, sizeof(tx_data1));
    circular_buffer_init(&rx_buffer2, rx_data2, sizeof(rx_data2));
    circular_buffer_init(&tx_buffer2, tx_data2, sizeof(tx_data2));

    // Set up expectations for init
    UART_LL_init_Expect(UART_BUS_0, rx_data1, sizeof(rx_data1));
    UART_LL_set_idle_callback_Ignore();
    UART_LL_set_rx_complete_callback_Ignore();
    UART_LL_set_tx_complete_callback_Ignore();

    UART_Handle *handle1 = UART_init(0, &rx_buffer1, &tx_buffer1);

    UART_LL_init_Expect(UART_BUS_1, rx_data2, sizeof(rx_data2));
    UART_LL_set_idle_callback_Ignore();
    UART_LL_set_rx_complete_callback_Ignore();
    UART_LL_set_tx_complete_callback_Ignore();

    UART_Handle *handle2 = UART_init(1, &rx_buffer2, &tx_buffer2);

    // Mock DMA position calls during passthrough enable
    UART_LL_get_dma_position_ExpectAndReturn(UART_BUS_0, 0);
    UART_LL_get_dma_position_ExpectAndReturn(UART_BUS_1, 0);

    // Enable passthrough first
    UART_enable_passthrough(handle1, handle2);

    // Act - Disable passthrough
    UART_disable_passthrough(handle1, handle2);

    // Assert - No errors thrown, passthrough disabled successfully (test passes if no exception)

    // Cleanup
    UART_LL_deinit_Ignore();
    UART_LL_set_idle_callback_Ignore();
    UART_LL_set_rx_complete_callback_Ignore();
    UART_LL_set_tx_complete_callback_Ignore();

    UART_deinit(handle1);
    UART_deinit(handle2);
}

void test_UART_disable_passthrough_invalid_pair(void)
{
    // Arrange - Initialize three UART buses
    CircularBuffer rx_buffer1, tx_buffer1;
    CircularBuffer rx_buffer2, tx_buffer2;
    CircularBuffer rx_buffer3, tx_buffer3;
    uint8_t rx_data1[256], tx_data1[256];
    uint8_t rx_data2[256], tx_data2[256];
    uint8_t rx_data3[256], tx_data3[256];

    // Claim bus 2 for this test (normally reserved by syscalls module)
    g_SYSCALLS_uart_claim = true;

    circular_buffer_init(&rx_buffer1, rx_data1, sizeof(rx_data1));
    circular_buffer_init(&tx_buffer1, tx_data1, sizeof(tx_data1));
    circular_buffer_init(&rx_buffer2, rx_data2, sizeof(rx_data2));
    circular_buffer_init(&tx_buffer2, tx_data2, sizeof(tx_data2));
    circular_buffer_init(&rx_buffer3, rx_data3, sizeof(rx_data3));
    circular_buffer_init(&tx_buffer3, tx_data3, sizeof(tx_data3));

    // Set up expectations for init
    UART_LL_init_Expect(UART_BUS_0, rx_data1, sizeof(rx_data1));
    UART_LL_set_idle_callback_Ignore();
    UART_LL_set_rx_complete_callback_Ignore();
    UART_LL_set_tx_complete_callback_Ignore();

    UART_Handle *handle1 = UART_init(0, &rx_buffer1, &tx_buffer1);

    UART_LL_init_Expect(UART_BUS_1, rx_data2, sizeof(rx_data2));
    UART_LL_set_idle_callback_Ignore();
    UART_LL_set_rx_complete_callback_Ignore();
    UART_LL_set_tx_complete_callback_Ignore();

    UART_Handle *handle2 = UART_init(1, &rx_buffer2, &tx_buffer2);

    UART_LL_init_Expect(UART_BUS_2, rx_data3, sizeof(rx_data3));
    UART_LL_set_idle_callback_Ignore();
    UART_LL_set_rx_complete_callback_Ignore();
    UART_LL_set_tx_complete_callback_Ignore();

    UART_Handle *handle3 = UART_init(2, &rx_buffer3, &tx_buffer3);

    // Mock DMA position calls during passthrough enable
    UART_LL_get_dma_position_ExpectAndReturn(UART_BUS_0, 0);
    UART_LL_get_dma_position_ExpectAndReturn(UART_BUS_1, 0);

    // Enable passthrough between handle1 and handle2
    UART_enable_passthrough(handle1, handle2);

    Error caught_error = ERROR_NONE;

    // Act - Try to disable passthrough with mismatched handles
    TRY {
        UART_disable_passthrough(handle1, handle3);
    } CATCH(caught_error) {
        // Expected to catch an error
    }

    // Assert
    TEST_ASSERT_EQUAL(ERROR_INVALID_ARGUMENT, caught_error);

    // Cleanup
    UART_disable_passthrough(handle1, handle2);

    UART_LL_deinit_Ignore();
    UART_LL_set_idle_callback_Ignore();
    UART_LL_set_rx_complete_callback_Ignore();
    UART_LL_set_tx_complete_callback_Ignore();

    UART_deinit(handle1);
    UART_deinit(handle2);
    UART_deinit(handle3);

    // Reset syscalls claim flag
    g_SYSCALLS_uart_claim = false;
}

void test_UART_enable_passthrough_already_active(void)
{
    // Arrange - Initialize four UART buses
    CircularBuffer rx_buffer1, tx_buffer1;
    CircularBuffer rx_buffer2, tx_buffer2;
    CircularBuffer rx_buffer3, tx_buffer3;
    uint8_t rx_data1[256], tx_data1[256];
    uint8_t rx_data2[256], tx_data2[256];
    uint8_t rx_data3[256], tx_data3[256];

    // Claim bus 2 for this test (normally reserved by syscalls module)
    g_SYSCALLS_uart_claim = true;

    circular_buffer_init(&rx_buffer1, rx_data1, sizeof(rx_data1));
    circular_buffer_init(&tx_buffer1, tx_data1, sizeof(tx_data1));
    circular_buffer_init(&rx_buffer2, rx_data2, sizeof(rx_data2));
    circular_buffer_init(&tx_buffer2, tx_data2, sizeof(tx_data2));
    circular_buffer_init(&rx_buffer3, rx_data3, sizeof(rx_data3));
    circular_buffer_init(&tx_buffer3, tx_data3, sizeof(tx_data3));

    // Set up expectations for init
    UART_LL_init_Expect(UART_BUS_0, rx_data1, sizeof(rx_data1));
    UART_LL_set_idle_callback_Ignore();
    UART_LL_set_rx_complete_callback_Ignore();
    UART_LL_set_tx_complete_callback_Ignore();

    UART_Handle *handle1 = UART_init(0, &rx_buffer1, &tx_buffer1);

    UART_LL_init_Expect(UART_BUS_1, rx_data2, sizeof(rx_data2));
    UART_LL_set_idle_callback_Ignore();
    UART_LL_set_rx_complete_callback_Ignore();
    UART_LL_set_tx_complete_callback_Ignore();

    UART_Handle *handle2 = UART_init(1, &rx_buffer2, &tx_buffer2);

    UART_LL_init_Expect(UART_BUS_2, rx_data3, sizeof(rx_data3));
    UART_LL_set_idle_callback_Ignore();
    UART_LL_set_rx_complete_callback_Ignore();
    UART_LL_set_tx_complete_callback_Ignore();

    UART_Handle *handle3 = UART_init(2, &rx_buffer3, &tx_buffer3);

    // Mock DMA position calls during passthrough enable
    UART_LL_get_dma_position_ExpectAndReturn(UART_BUS_0, 0);
    UART_LL_get_dma_position_ExpectAndReturn(UART_BUS_1, 0);

    // Enable passthrough between handle1 and handle2
    UART_enable_passthrough(handle1, handle2);

    Error caught_error = ERROR_NONE;

    // Act - Try to enable another passthrough while one is active
    TRY {
        UART_enable_passthrough(handle1, handle3);
    } CATCH(caught_error) {
        // Expected to catch an error
    }

    // Assert - Should fail because only one passthrough pair is allowed at a time
    TEST_ASSERT_EQUAL(ERROR_RESOURCE_BUSY, caught_error);

    // Cleanup
    UART_disable_passthrough(handle1, handle2);

    UART_LL_deinit_Ignore();
    UART_LL_set_idle_callback_Ignore();
    UART_LL_set_rx_complete_callback_Ignore();
    UART_LL_set_tx_complete_callback_Ignore();

    UART_deinit(handle1);
    UART_deinit(handle2);
    UART_deinit(handle3);

    // Reset syscalls claim flag
    g_SYSCALLS_uart_claim = false;
}

void test_UART_deinit_with_active_passthrough(void)
{
    // Arrange - Initialize two UART buses
    CircularBuffer rx_buffer1, tx_buffer1;
    CircularBuffer rx_buffer2, tx_buffer2;
    uint8_t rx_data1[256], tx_data1[256];
    uint8_t rx_data2[256], tx_data2[256];

    circular_buffer_init(&rx_buffer1, rx_data1, sizeof(rx_data1));
    circular_buffer_init(&tx_buffer1, tx_data1, sizeof(tx_data1));
    circular_buffer_init(&rx_buffer2, rx_data2, sizeof(rx_data2));
    circular_buffer_init(&tx_buffer2, tx_data2, sizeof(tx_data2));

    // Set up expectations for init
    UART_LL_init_Expect(UART_BUS_0, rx_data1, sizeof(rx_data1));
    UART_LL_set_idle_callback_Ignore();
    UART_LL_set_rx_complete_callback_Ignore();
    UART_LL_set_tx_complete_callback_Ignore();

    UART_Handle *handle1 = UART_init(0, &rx_buffer1, &tx_buffer1);

    UART_LL_init_Expect(UART_BUS_1, rx_data2, sizeof(rx_data2));
    UART_LL_set_idle_callback_Ignore();
    UART_LL_set_rx_complete_callback_Ignore();
    UART_LL_set_tx_complete_callback_Ignore();

    UART_Handle *handle2 = UART_init(1, &rx_buffer2, &tx_buffer2);

    // Mock DMA position calls during passthrough enable
    UART_LL_get_dma_position_ExpectAndReturn(UART_BUS_0, 0);
    UART_LL_get_dma_position_ExpectAndReturn(UART_BUS_1, 0);

    // Enable passthrough
    UART_enable_passthrough(handle1, handle2);

    Error caught_error = ERROR_NONE;

    // Act - Try to deinit while passthrough is active
    TRY {
        UART_deinit(handle1);
    } CATCH(caught_error) {
        // Expected to catch an error
    }

    // Assert - Should fail because passthrough is active
    TEST_ASSERT_EQUAL(ERROR_RESOURCE_BUSY, caught_error);

    // Cleanup - Properly disable passthrough before deinit
    UART_disable_passthrough(handle1, handle2);

    UART_LL_deinit_Ignore();
    UART_LL_set_idle_callback_Ignore();
    UART_LL_set_rx_complete_callback_Ignore();
    UART_LL_set_tx_complete_callback_Ignore();

    UART_deinit(handle1);
    UART_deinit(handle2);
}

void test_UART_enable_passthrough_with_same_handle(void)
{
    // Arrange - Initialize one UART bus
    CircularBuffer rx_buffer, tx_buffer;
    uint8_t rx_data[256], tx_data[256];

    circular_buffer_init(&rx_buffer, rx_data, sizeof(rx_data));
    circular_buffer_init(&tx_buffer, tx_data, sizeof(tx_data));

    // Set up expectations for init
    UART_LL_init_Expect(UART_BUS_0, rx_data, sizeof(rx_data));
    UART_LL_set_idle_callback_Ignore();
    UART_LL_set_rx_complete_callback_Ignore();
    UART_LL_set_tx_complete_callback_Ignore();

    UART_Handle *handle = UART_init(0, &rx_buffer, &tx_buffer);
    TEST_ASSERT_NOT_NULL(handle);

    Error caught_error = ERROR_NONE;

    // Act - Try to enable passthrough with the same handle for both buses
    TRY {
        UART_enable_passthrough(handle, handle);
    } CATCH(caught_error) {
        // Expected to catch an error
    }

    // Assert
    TEST_ASSERT_EQUAL(ERROR_INVALID_ARGUMENT, caught_error);

    // Cleanup
    UART_LL_deinit_Ignore();
    UART_LL_set_idle_callback_Ignore();
    UART_LL_set_rx_complete_callback_Ignore();
    UART_LL_set_tx_complete_callback_Ignore();

    UART_deinit(handle);
}
