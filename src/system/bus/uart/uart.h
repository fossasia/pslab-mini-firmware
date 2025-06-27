/**
 * @file uart.h
 * @brief UART (Universal Asynchronous Receiver/Transmitter) driver interface
 *
 * This module provides a non-blocking UART driver with callback support
 * for protocol implementations. All operations are interrupt- and DMA-driven
 * for optimal performance.
 *
 * Features:
 * - Non-blocking read/write operations
 * - Configurable RX callback for protocol implementations
 * - Buffer status inquiry functions
 *
 * Basic Usage:
 * @code
 * // Initialize UART
 * UART_init();
 *
 * // Simple echo example
 * if (UART_rx_ready()) {
 *     uint8_t buffer[32];
 *     uint32_t bytes = UART_read(buffer, sizeof(buffer));
 *     UART_write(buffer, bytes);
 * }
 *
 * // Protocol with header + payload
 * void on_header_ready(uint32_t available) {
 *     header_flag = true;  // Signal main loop
 * }
 *
 * UART_set_rx_callback(on_header_ready, HEADER_SIZE);
 * @endcode
 *
 * @author Alexander Bessman
 * @date 2025-06-27
 */

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
 * @return Number of bytes actually read.
 */
uint32_t UART_read(uint8_t *rxbuf, uint32_t sz);

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
 * @brief Get number of bytes available in receive buffer.
 *
 * @return Number of bytes available to read.
 */
uint32_t UART_rx_available(void);

/**
 * @brief Callback function type for RX data availability.
 *
 * @param bytes_available Number of bytes currently available in RX buffer.
 */
typedef void (*uart_rx_callback_t)(uint32_t bytes_available);

/**
 * @brief Set RX callback to be triggered when threshold bytes are available.
 *
 * @param callback Function to call when threshold is reached (NULL to disable)
 * @param threshold Number of bytes that must be available to trigger callback
 */
void UART_set_rx_callback(uart_rx_callback_t callback, uint32_t threshold);

/**
 * @brief Get TX buffer free space.
 *
 * @return Number of bytes that can still be written to TX buffer.
 */
uint32_t UART_tx_free_space(void);

/**
 * @brief Check if TX transmission is in progress.
 *
 * @return true if transmission is ongoing, false otherwise.
 */
bool UART_tx_busy(void);

#ifdef __cplusplus
}
#endif

#endif /* UART_H */
