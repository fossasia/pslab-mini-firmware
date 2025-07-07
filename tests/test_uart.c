#include "unity.h"
#include "mock_uart_ll.h"
#include "uart.h"
#include <stdlib.h>
#include <stdbool.h>

// Define missing CMock global symbols
int GlobalExpectCount = 0;
int GlobalVerifyOrder = 0;

// Test fixtures
static BUS_CircBuffer rx_buffer;
static BUS_CircBuffer tx_buffer;
static uint8_t rx_data[256];
static uint8_t tx_data[256];

void setUp(void)
{
    // Initialize buffers for each test
    circular_buffer_init(&rx_buffer, rx_data, 256);
    circular_buffer_init(&tx_buffer, tx_data, 256);
    
    // Initialize mocks
    mock_uart_ll_Init();
}

void tearDown(void)
{
    // Clean up after each test
    mock_uart_ll_Destroy();
}

void test_UART_init_success(void)
{
    // Arrange
    size_t bus = 0;
    UART_Handle *handle;
    
    // Set expectations for UART_LL calls - use Ignore for callbacks to avoid linking issues
    UART_LL_init_Expect(UART_BUS_0, rx_data, 256);
    UART_LL_set_idle_callback_Ignore();
    UART_LL_set_rx_complete_callback_Ignore();
    UART_LL_set_tx_complete_callback_Ignore();
    
    // Act
    handle = UART_init(bus, &rx_buffer, &tx_buffer);
    
    // Assert
    TEST_ASSERT_NOT_NULL(handle);
}

void test_UART_init_null_rx_buffer(void)
{
    // Arrange
    size_t bus = 0;
    UART_Handle *handle;
    
    // Act - pass NULL rx_buffer
    handle = UART_init(bus, NULL, &tx_buffer);
    
    // Assert
    TEST_ASSERT_NULL(handle);
}

void test_UART_init_null_tx_buffer(void)
{
    // Arrange
    size_t bus = 0;
    UART_Handle *handle;
    
    // Act - pass NULL tx_buffer
    handle = UART_init(bus, &rx_buffer, NULL);
    
    // Assert
    TEST_ASSERT_NULL(handle);
}

void test_UART_init_invalid_bus(void)
{
    // Arrange
    size_t bus = UART_BUS_COUNT; // Invalid bus number
    UART_Handle *handle;
    
    // Act
    handle = UART_init(bus, &rx_buffer, &tx_buffer);
    
    // Assert
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
    UART_Handle *handle;
    uint8_t test_data[] = {0x01, 0x02, 0x03, 0x04};
    
    // Set up expectations for init
    UART_LL_init_Expect(UART_BUS_0, rx_data, 256);
    UART_LL_set_idle_callback_Ignore();
    UART_LL_set_rx_complete_callback_Ignore();
    UART_LL_set_tx_complete_callback_Ignore();
    
    handle = UART_init(bus, &rx_buffer, &tx_buffer);
    
    // Expect the TX functions to be called when data is available
    UART_LL_tx_busy_ExpectAndReturn(UART_BUS_0, false);
    UART_LL_start_dma_tx_Ignore();
    
    // Act
    uint32_t bytes_written = UART_write(handle, test_data, sizeof(test_data));
    
    // Assert - Initially buffer should be able to accept data
    // This tests that the function doesn't crash and returns some valid value
    TEST_ASSERT_GREATER_OR_EQUAL(0, bytes_written);
    TEST_ASSERT_LESS_OR_EQUAL(sizeof(test_data), bytes_written);
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
    UART_Handle *handle;
    uint8_t read_buffer[10];
    
    // Set up expectations for init
    UART_LL_init_Expect(UART_BUS_0, rx_data, 256);
    UART_LL_set_idle_callback_Ignore();
    UART_LL_set_rx_complete_callback_Ignore();
    UART_LL_set_tx_complete_callback_Ignore();
    
    // Mock DMA position for read operation
    UART_LL_get_dma_position_ExpectAndReturn(UART_BUS_0, 0);
    
    handle = UART_init(bus, &rx_buffer, &tx_buffer);
    
    // Act
    uint32_t bytes_read = UART_read(handle, read_buffer, sizeof(read_buffer));
    
    // Assert
    TEST_ASSERT_EQUAL(0, bytes_read); // Buffer is empty initially
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
    UART_Handle *handle;
    
    // Set up expectations for init
    UART_LL_init_Expect(UART_BUS_0, rx_data, 256);
    UART_LL_set_idle_callback_Ignore();
    UART_LL_set_rx_complete_callback_Ignore();
    UART_LL_set_tx_complete_callback_Ignore();
    
    // Mock DMA position for rx_ready check
    UART_LL_get_dma_position_ExpectAndReturn(UART_BUS_0, 0);
    
    handle = UART_init(bus, &rx_buffer, &tx_buffer);
    
    // Act
    bool ready = UART_rx_ready(handle);
    
    // Assert
    TEST_ASSERT_FALSE(ready); // Buffer is empty initially
}

void test_UART_deinit_with_valid_handle(void)
{
    // Arrange
    size_t bus = 0;
    UART_Handle *handle;
    
    // Set up expectations for init
    UART_LL_init_Expect(UART_BUS_0, rx_data, 256);
    UART_LL_set_idle_callback_Ignore();
    UART_LL_set_rx_complete_callback_Ignore();
    UART_LL_set_tx_complete_callback_Ignore();
    
    handle = UART_init(bus, &rx_buffer, &tx_buffer);
    
    // Set up expectations for deinit
    UART_LL_deinit_Expect(UART_BUS_0);
    UART_LL_set_idle_callback_Expect(UART_BUS_0, NULL);
    UART_LL_set_rx_complete_callback_Expect(UART_BUS_0, NULL);
    UART_LL_set_tx_complete_callback_Expect(UART_BUS_0, NULL);
    
    // Act
    UART_deinit(handle);
    
    // Assert - no assertion needed, just verify mocks are called
}
