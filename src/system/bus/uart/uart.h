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

/**
 * @brief Receive data from UART using DMA.
 *
 * This function initiates a DMA transfer to receive a specified number of bytes
 * into the provided buffer over the UART peripheral.
 *
 * @param rxbuf Pointer to allocated memory into which to read data.
 * @param sz    Number of bytes to read into the buffer.
 * @return HAL status (HAL_OK on success).
 */
HAL_StatusTypeDef UART_read_DMA(uint8_t *rxbuf, uint32_t sz);

/**
 * @brief Transmit data over UART using DMA.
 *
 * This function initiates a DMA transfer to transmit a specified number of bytes
 * from the provided buffer over the UART peripheral.
 *
 * @param txbuf Pointer to the data buffer to be transmitted.
 * @param sz    Number of bytes to write from the buffer.
 * @return HAL status (HAL_OK on success).
 */
HAL_StatusTypeDef UART_write_DMA(uint8_t const *txbuf, uint32_t sz);

extern DMA_HandleTypeDef hdma_usart3_rx;
extern DMA_HandleTypeDef hdma_usart3_tx;

extern uint8_t tx_buffer[10];
extern uint8_t rx_buffer[10];

extern uint32_t rx_counter, tx_counter;

#ifdef __cplusplus
}
#endif

#endif /* UART_H */
