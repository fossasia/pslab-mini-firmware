#ifndef PSLAB_APPLICATION_GATEWAY_UART_COMMANDS_H
#define PSLAB_APPLICATION_GATEWAY_UART_COMMANDS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    UART_GATEWAY_DEFAULT_BUS = 1,
    UART_GATEWAY_DEFAULT_BAUD = 115200,
    UART_GATEWAY_DEFAULT_TIMEOUT_MS = 100,
    UART_GATEWAY_MAX_TRANSFER = 512,
};

bool uart_gateway_set_bus(uint32_t bus);
uint32_t uart_gateway_get_bus(void);

bool uart_gateway_set_baud(uint32_t baud);
uint32_t uart_gateway_get_baud(void);

bool uart_gateway_set_timeout(uint32_t timeout_ms);
uint32_t uart_gateway_get_timeout(void);

bool uart_gateway_open(void);
void uart_gateway_close(void);
bool uart_gateway_is_open(void);

uint32_t uart_gateway_write(uint8_t const *data, size_t len);
uint32_t uart_gateway_read(uint8_t *data, size_t max_len);
uint32_t uart_gateway_available(void);
void uart_gateway_clear(void);
bool uart_gateway_flush(void);
uint32_t uart_gateway_transact(
    uint8_t const *tx_data,
    size_t tx_len,
    uint8_t *rx_data,
    size_t rx_max_len
);

#ifdef __cplusplus
}
#endif

#endif
