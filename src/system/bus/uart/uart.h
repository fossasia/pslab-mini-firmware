#ifndef UART_H
#define UART_H

#include <stdbool.h>
#include <stdint.h>

#include "stm32h5xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the UART peripheral.
 *
 * Configures baud rate, data bits, stop bits, parity and enables TX/RX.
 *
 * @return HAL status (HAL_OK on success).
 */
HAL_StatusTypeDef UART_init(void);

/**
 * @brief Transmit data over UART.
 *
 * @param txbuf Pointer to data buffer to transmit.
 * @param sz    Number of bytes to send.
 * @return HAL status (HAL_OK on success).
 */
HAL_StatusTypeDef UART_write(uint8_t const *txbuf, uint32_t sz);

/**
 * @brief Receive data from UART.
 *
 * @param rxbuf Pointer to buffer to store received data.
 * @param sz    Number of bytes to read.
 * @return HAL status (HAL_OK on success).
 */
HAL_StatusTypeDef UART_read(uint8_t *rxbuf, uint32_t sz);

/**
 * @brief Check if data is available to read from the UART receive buffer.
 *
 * This function returns true if the UART receive data register is not empty,
 * indicating that there is unread data available to be received.
 *
 * @return true if at least one byte is available to be read, false otherwise.
 */
bool UART_rx_ready(void);

#ifdef __cplusplus
}
#endif

#endif /* UART_H */
