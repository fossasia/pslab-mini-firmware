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
 * // Initialize UART with handle and buffers
 * UART_Handle *uart_handle;
 * CircularBuffer rx_buffer, tx_buffer;
 * uint8_t rx_data[256], tx_data[256];
 *
 * circular_buffer_init(&rx_buffer, rx_data, 256);
 * circular_buffer_init(&tx_buffer, tx_data, 256);
 * uart_handle = UART_init(0, &rx_buffer, &tx_buffer);
 * if (uart_handle == nullptr) {
 *     // Initialization failed
 *     return;
 * }
 *
 * // Simple echo example
 * if (UART_rx_ready(uart_handle)) {
 *     uint8_t buffer[32];
 *     uint32_t bytes = UART_read(uart_handle, buffer, sizeof(buffer));
 *     UART_write(uart_handle, buffer, bytes);
 * }
 *
 * // Protocol with header + payload
 * void on_header_ready(UART_Handle *handle, uint32_t available) {
 *     header_flag = true;  // Signal main loop
 * }
 *
 * UART_set_rx_callback(uart_handle, on_header_ready, HEADER_SIZE);
 *
 * // Cleanup when done
 * UART_deinit(uart_handle);
 * @endcode
 *
 * @author Alexander Bessman
 * @date 2025-06-27
 */

#ifndef UART_H
#define UART_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "util/util.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief UART bus handle structure
 */
typedef struct UART_Handle UART_Handle;

/**
 * @brief Callback function type for RX data availability.
 *
 * @param handle Pointer to UART handle structure
 * @param bytes_available Number of bytes currently available in RX buffer.
 */
typedef void (*UART_RxCallback)(UART_Handle *handle, uint32_t bytes_available);

/**
 * @brief Get the number of available UART bus instances.
 *
 * @return Number of UART buses supported by this platform
 */
size_t UART_get_bus_count(void);

/**
 * @brief Initialize the UART peripheral.
 *
 * Configures baud rate, data bits, stop bits, parity and enables TX/RX.
 * Allocates and returns a new UART handle.
 *
 * @param bus UART bus instance to initialize (0-based index)
 * @param rx_buffer Pointer to pre-allocated RX circular buffer
 * @param tx_buffer Pointer to pre-allocated TX circular buffer
 * @return Pointer to UART handle on success, nullptr on failure (including
 *         invalid bus number)
 */
UART_Handle *UART_init(
    size_t bus,
    CircularBuffer *rx_buffer,
    CircularBuffer *tx_buffer
);

/**
 * @brief Deinitialize the UART peripheral.
 *
 * @param handle Pointer to UART handle structure
 */
void UART_deinit(UART_Handle *handle);

/**
 * @brief Transmit data over UART.
 *
 * @param handle Pointer to UART handle structure
 * @param txbuf Pointer to data buffer to transmit.
 * @param sz    Number of bytes to send.
 * @return Number of bytes actually written.
 */
uint32_t UART_write(UART_Handle *handle, uint8_t const *txbuf, uint32_t sz);

/**
 * @brief Receive data from UART.
 *
 * @param handle Pointer to UART handle structure
 * @param rxbuf Pointer to buffer to store received data.
 * @param sz    Number of bytes to read.
 * @return Number of bytes actually read.
 */
uint32_t UART_read(UART_Handle *handle, uint8_t *rxbuf, uint32_t sz);

/**
 * @brief Flush the UART transmit buffer
 *
 * Wait for the UART transmit buffer to be empty or until the specified timeout
 * period elapses.
 *
 * @note If timeout is zero, the function will wait indefinitely for the buffer
 *       to be flushed.
 *
 * @param handle Pointer to UART handle structure
 * @param timeout Timeout period in milliseconds, 0 for infinite
 * @return true if the buffer was flushed successfully, false on timeout
 */
bool UART_flush(UART_Handle *handle, uint32_t timeout);

/**
 * @brief Check if data is available to read from the UART receive buffer.
 *
 * This function returns true if the UART receive data register is not empty,
 * indicating that there is unread data available to be received.
 *
 * @param handle Pointer to UART handle structure
 * @return true if at least one byte is available to be read, false otherwise.
 */
bool UART_rx_ready(UART_Handle *handle);

/**
 * @brief Get number of bytes available in receive buffer.
 *
 * @param handle Pointer to UART handle structure
 * @return Number of bytes available to read.
 */
uint32_t UART_rx_available(UART_Handle *handle);

/**
 * @brief Set RX callback to be triggered when threshold bytes are available.
 *
 * @param handle Pointer to UART handle structure
 * @param callback Function to call when threshold is reached (nullptr to
 * disable)
 * @param threshold Number of bytes that must be available to trigger callback
 */
void UART_set_rx_callback(
    UART_Handle *handle,
    UART_RxCallback callback,
    uint32_t threshold
);

/**
 * @brief Get TX buffer free space.
 *
 * @param handle Pointer to UART handle structure
 * @return Number of bytes that can still be written to TX buffer.
 */
uint32_t UART_tx_free_space(UART_Handle *handle);

/**
 * @brief Check if TX transmission is in progress.
 *
 * @param handle Pointer to UART handle structure
 * @return true if transmission is ongoing, false otherwise.
 */
bool UART_tx_busy(UART_Handle *handle);

#ifdef __cplusplus
}
#endif

#endif /* UART_H */
