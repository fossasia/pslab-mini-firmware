#include "unity.h"
#include "bus.h"
#include "uart.h"
#include "uart_ll.h"
#include "mock_uart_ll.h"

void setUp(void)
{
    // Reset mock state before each test
    mock_uart_ll_reset();
}

void tearDown(void)
{
}

void test_UART_init_success(void)
{
    // Arrange
    uint8_t rx_buf[256];
    uint8_t tx_buf[256];
    circular_buffer_t rx_buffer = {rx_buf, sizeof(rx_buf), 0, 0};
    circular_buffer_t tx_buffer = {tx_buf, sizeof(tx_buf), 0, 0};
    
    // Act
    uart_handle_t *handle = UART_init(0, &rx_buffer, &tx_buffer);
    
    // Assert
    TEST_ASSERT_NOT_NULL(handle);
    TEST_ASSERT_TRUE(mock_uart_ll_init_called());
    
    // Cleanup
    UART_deinit(handle);
}

void test_UART_init_returns_same_handle_for_multiple_calls(void)
{
    // Arrange
    uint8_t rx_buf[256];
    uint8_t tx_buf[256];
    circular_buffer_t rx_buffer = {rx_buf, sizeof(rx_buf), 0, 0};
    circular_buffer_t tx_buffer = {tx_buf, sizeof(tx_buf), 0, 0};
    
    // Act
    uart_handle_t *handle1 = UART_init(0, &rx_buffer, &tx_buffer);
    uart_handle_t *handle2 = UART_init(0, &rx_buffer, &tx_buffer);
    
    // Assert
    TEST_ASSERT_NOT_NULL(handle1);
    TEST_ASSERT_NULL(handle2); // Should fail - bus already initialized
    
    // Cleanup
    UART_deinit(handle1);
}

void test_UART_init_null_rx_buffer_fails(void)
{
    // Arrange
    uint8_t tx_buf[256];
    circular_buffer_t tx_buffer = {tx_buf, sizeof(tx_buf), 0, 0};
    
    // Act
    uart_handle_t *handle = UART_init(0, NULL, &tx_buffer);
    
    // Assert
    TEST_ASSERT_NULL(handle);
}

void test_UART_init_null_tx_buffer_fails(void)
{
    // Arrange
    uint8_t rx_buf[256];
    circular_buffer_t rx_buffer = {rx_buf, sizeof(rx_buf), 0, 0};
    
    // Act
    uart_handle_t *handle = UART_init(0, &rx_buffer, NULL);
    
    // Assert
    TEST_ASSERT_NULL(handle);
}

void test_UART_init_invalid_bus_fails(void)
{
    // Arrange
    uint8_t rx_buf[256];
    uint8_t tx_buf[256];
    circular_buffer_t rx_buffer = {rx_buf, sizeof(rx_buf), 0, 0};
    circular_buffer_t tx_buffer = {tx_buf, sizeof(tx_buf), 0, 0};
    
    // Act
    uart_handle_t *handle = UART_init(UART_BUS_COUNT + 1, &rx_buffer, &tx_buffer);
    
    // Assert
    TEST_ASSERT_NULL(handle);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_UART_init_success);
    RUN_TEST(test_UART_init_returns_same_handle_for_multiple_calls);
    RUN_TEST(test_UART_init_null_rx_buffer_fails);
    RUN_TEST(test_UART_init_null_tx_buffer_fails);
    RUN_TEST(test_UART_init_invalid_bus_fails);
    return UNITY_END();
}
