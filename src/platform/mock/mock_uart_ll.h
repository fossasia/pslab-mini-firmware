#ifndef MOCK_UART_LL_H
#define MOCK_UART_LL_H

#include <stdbool.h>
#include <stdint.h>

#include "uart_ll.h"

// Mock control functions
void mock_uart_ll_reset(void);

// Mock state query functions
bool mock_uart_ll_init_called(void);
bool mock_uart_ll_deinit_called(void);
uint32_t mock_uart_ll_get_last_tx_size(void);
uint8_t *mock_uart_ll_get_last_tx_buffer(void);
uart_bus_t mock_uart_ll_get_last_bus(void);

// Mock state control functions
void mock_uart_ll_set_tx_busy(bool busy);
void mock_uart_ll_set_dma_position(uint32_t position);

// Mock callback trigger functions
void mock_uart_ll_trigger_tx_complete(uart_bus_t bus, uint32_t bytes);
void mock_uart_ll_trigger_rx_complete(uart_bus_t bus);
void mock_uart_ll_trigger_idle(uart_bus_t bus, uint32_t dma_pos);

#endif // MOCK_UART_LL_H
