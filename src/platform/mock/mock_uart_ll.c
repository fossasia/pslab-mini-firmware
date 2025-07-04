#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "uart_ll.h"

static bool init_was_called = false;
static bool deinit_was_called = false;
static bool tx_busy_state = false;
static uint32_t dma_position = 0;
static uint32_t last_tx_size = 0;
static uint8_t *last_tx_buffer = nullptr;
static uart_bus_t last_bus_used = UART_BUS_COUNT;
static UART_LL_idle_callback_t stored_idle_callback = nullptr;
static UART_LL_rx_complete_callback_t stored_rx_callback = nullptr;
static UART_LL_tx_complete_callback_t stored_tx_callback = nullptr;

void mock_uart_ll_reset(void) {
    init_was_called = false;
    deinit_was_called = false;
    tx_busy_state = false;
    dma_position = 0;
    last_tx_size = 0;
    last_tx_buffer = nullptr;
    last_bus_used = UART_BUS_COUNT;
    stored_idle_callback = nullptr;
    stored_rx_callback = nullptr;
    stored_tx_callback = nullptr;
}

bool mock_uart_ll_init_called(void) {
    // Track if init was called
    return init_was_called;
}

bool mock_uart_ll_deinit_called(void) {
    return deinit_was_called;
}

void mock_uart_ll_set_tx_busy(bool busy) {
    tx_busy_state = busy;
}

void mock_uart_ll_set_dma_position(uint32_t position) {
    dma_position = position;
}

uint32_t mock_uart_ll_get_last_tx_size(void) {
    return last_tx_size;
}

uint8_t *mock_uart_ll_get_last_tx_buffer(void) {
    return last_tx_buffer;
}

uart_bus_t mock_uart_ll_get_last_bus(void) {
    return last_bus_used;
}

void mock_uart_ll_trigger_tx_complete(uart_bus_t bus, uint32_t bytes) {
    if (stored_tx_callback != nullptr) {
        stored_tx_callback(bus, bytes);
    }
}

void mock_uart_ll_trigger_rx_complete(uart_bus_t bus) {
    if (stored_rx_callback != nullptr) {
        stored_rx_callback(bus);
    }
}

void mock_uart_ll_trigger_idle(uart_bus_t bus, uint32_t dma_pos) {
    if (stored_idle_callback != nullptr) {
        stored_idle_callback(bus, dma_pos);
    }
}

void UART_LL_init(uart_bus_t bus, uint8_t *buffer, uint32_t size) {
    if (!init_was_called) {
        init_was_called = true;
    }
    last_bus_used = bus;
}

void UART_LL_deinit(uart_bus_t bus) {
    deinit_was_called = true;
    last_bus_used = bus;
}

uint32_t UART_LL_get_dma_position(uart_bus_t bus) {
    last_bus_used = bus;
    return dma_position;
}

bool UART_LL_tx_busy(uart_bus_t bus) {
    last_bus_used = bus;
    return tx_busy_state;
}

void UART_LL_start_dma_tx(uart_bus_t bus, uint8_t *buffer, uint32_t size) {
    last_bus_used = bus;
    last_tx_buffer = buffer;
    last_tx_size = size;
    tx_busy_state = true;  // Automatically set busy when starting transmission
}

void UART_LL_set_idle_callback(uart_bus_t bus, UART_LL_idle_callback_t callback) {
    last_bus_used = bus;
    stored_idle_callback = callback;
}

void UART_LL_set_rx_complete_callback(uart_bus_t bus, UART_LL_rx_complete_callback_t callback) {
    last_bus_used = bus;
    stored_rx_callback = callback;
}

void UART_LL_set_tx_complete_callback(uart_bus_t bus, UART_LL_tx_complete_callback_t callback) {
    last_bus_used = bus;
    stored_tx_callback = callback;
}
