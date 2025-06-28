/**
 * @file uart.c
 * @brief Hardware-independent UART (Universal Asynchronous Receiver/Transmitter) implementation
 *
 * This module provides the hardware-independent layer of the UART driver.
 * It manages circular buffers for transmission and reception, handles callbacks,
 * and provides the public API exposed through uart.h.
 *
 * Features:
 * - Non-blocking read/write operations with circular buffers
 * - Configurable RX callback for protocol implementations
 * - Buffer status inquiry functions
 * - Circular buffer implementation with automatic wrap-around
 *
 * This implementation relies on hardware-specific functions defined in
 * src/system/h563xx/uart.c (or equivalent for other platforms).
 *
 * @author Alexander Bessman
 * @date 2025-06-28
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "bus_common.h"
#include "uart.h"
#include "uart_internal.h"

/* Buffer sizes - must be powers of 2 */
#define UART_RX_BUFFER_SIZE 256
#define UART_TX_BUFFER_SIZE 256

/* Buffers and their management structures */
static uint8_t rx_buffer_data[UART_RX_BUFFER_SIZE];
static uint8_t tx_buffer_data[UART_TX_BUFFER_SIZE];
static circular_buffer_t rx_buffer;
static circular_buffer_t tx_buffer;

/* RX DMA state */
static uint32_t volatile rx_dma_head = 0;

/* Callback state */
static uart_rx_callback_t rx_callback = NULL;
static uint32_t rx_threshold = 0;

/* Forward declarations of hardware-specific functions */
void UART_set_dma_buffer(uint8_t *buffer, uint32_t size);
void UART_start_dma_tx(uint8_t *buffer, uint32_t size);
uint32_t UART_get_dma_position(void);
bool UART_hw_tx_busy(void);


/**
 * @brief Buffer initialization called from hardware-specific init
 *
 * @param buffer Buffer to use for RX DMA (ignored, we use our own)
 * @param size Size of the buffer (ignored)
 */
void UART_buffer_init(void)
{
    /* Initialize circular buffers */
    circular_buffer_init(&rx_buffer, rx_buffer_data, UART_RX_BUFFER_SIZE);
    circular_buffer_init(&tx_buffer, tx_buffer_data, UART_TX_BUFFER_SIZE);

    /* Set the DMA buffer in the hardware layer */
    UART_set_dma_buffer(rx_buffer_data, UART_RX_BUFFER_SIZE);
}

/**
 * @brief Get number of bytes available in TX buffer.
 */
static uint32_t tx_buffer_available(void)
{
    return circular_buffer_available(&tx_buffer);
}

/**
 * @brief Get number of bytes available in RX buffer.
 */
static uint32_t rx_buffer_available(void)
{
    /* Get current DMA write position */
    uint32_t dma_pos = UART_get_dma_position();
    uint32_t dma_head = dma_pos;

    /* Update our cached head position */
    rx_dma_head = dma_head;

    /* Update the circular buffer's head position based on DMA */
    rx_buffer.head = dma_head;

    /* Calculate available bytes using common function */
    return circular_buffer_available(&rx_buffer);
}

/**
 * @brief Start transmission if not already in progress.
 */
static void start_transmission(void)
{
    if (UART_hw_tx_busy() || circular_buffer_is_empty(&tx_buffer)) {
        return;
    }

    /* Calculate how many contiguous bytes we can send */
    uint32_t available = tx_buffer_available();
    uint32_t contiguous_bytes;

    if (tx_buffer.tail <= tx_buffer.head) {
        contiguous_bytes = tx_buffer.head - tx_buffer.tail;
    } else {
        contiguous_bytes = tx_buffer.size - tx_buffer.tail;
    }

    /* Limit to available bytes and reasonable DMA transfer size */
    if (contiguous_bytes > available) {
        contiguous_bytes = available;
    }
    /* Limit DMA transfer size */
    if (contiguous_bytes > (UART_TX_BUFFER_SIZE / 2)) {
        contiguous_bytes = UART_TX_BUFFER_SIZE / 2;
    }

    if (contiguous_bytes > 0) {
        /* Start hardware TX DMA */
        UART_start_dma_tx(&tx_buffer.buffer[tx_buffer.tail], contiguous_bytes);
    }
}

/**
 * @brief Callback invoked by hardware layer when TX is complete
 *
 * @param bytes_transferred Number of bytes transferred
 */
void UART_tx_complete_callback(uint32_t bytes_transferred)
{
    /* Update TX buffer tail with the number of bytes that were sent */
    tx_buffer.tail = (tx_buffer.tail + bytes_transferred) % tx_buffer.size;

    /* Try to start another transmission if there's more data */
    start_transmission();
}

/**
 * @brief Check if RX callback condition is met and call if needed
 */
static bool check_rx_callback(void)
{
    if (rx_callback && rx_buffer_available() >= rx_threshold) {
        rx_callback(rx_buffer_available());
        return true;
    }
    return false;
}

/**
 * @brief Callback invoked by hardware layer when RX DMA buffer is full
 *
 * @param buffer_size Size of the buffer
 */
void UART_rx_complete_callback(uint32_t buffer_size)
{
    /* In circular DMA mode, just check for callbacks */
    check_rx_callback();
}

/**
 * @brief Callback invoked by hardware layer on UART idle line detection
 *
 * @param dma_pos Current DMA position
 */
void UART_idle_callback(uint32_t dma_pos)
{
    /* Update our cached DMA position */
    rx_dma_head = dma_pos;
    rx_buffer.head = dma_pos;

    /* Check for callbacks */
    check_rx_callback();
}

/**
 * @brief Write data to the UART interface.
 *
 * This function queues bytes for transmission using a circular buffer.
 * Data is sent via interrupt-driven transmission.
 *
 * @param txbuf      Pointer to the data buffer to be transmitted.
 * @param sz         Number of bytes to write from the buffer.
 *
 * @return Number of bytes actually written.
 */
uint32_t UART_write(uint8_t const *txbuf, uint32_t const sz)
{
    if (txbuf == NULL || sz == 0) {
        return 0;
    }

    /* Queue bytes for transmission */
    uint32_t bytes_written = circular_buffer_write(&tx_buffer, txbuf, sz);

    /* Start transmission if not already in progress */
    start_transmission();

    return bytes_written;
}

/**
 * @brief Read data from the UART interface.
 *
 * This function reads bytes from the receive circular buffer.
 * Returns the number of bytes actually read.
 *
 * @param rxbuf      Pointer to allocated memory into which to read data.
 * @param sz         Number of bytes to read into the buffer.
 *
 * @return Number of bytes actually read.
 */
uint32_t UART_read(uint8_t *const rxbuf, uint32_t const sz)
{
    if (rxbuf == NULL || sz == 0) {
        return 0;
    }

    uint32_t available = rx_buffer_available();
    uint32_t to_read = sz > available ? available : sz;

    /* Read from circular buffer using common function */
    return circular_buffer_read(&rx_buffer, rxbuf, to_read);
}

/**
 * @brief Check if data is available to read from the UART receive buffer.
 *
 * This function returns true if there is data available in the receive buffer,
 * indicating that there is unread data available to be received.
 *
 * @return true if at least one byte is available to be read, false otherwise.
 */
bool UART_rx_ready(void)
{
    return rx_buffer_available() > 0;
}

/**
 * @brief Get number of bytes available in receive buffer.
 *
 * This function returns the number of bytes currently available
 * to read from the receive buffer.
 *
 * @return Number of bytes available to read.
 */
uint32_t UART_rx_available(void)
{
    return rx_buffer_available();
}

/**
 * @brief Get TX buffer free space.
 *
 * @return Number of bytes that can still be written to TX buffer.
 */
uint32_t UART_tx_free_space(void)
{
    uint32_t used = tx_buffer_available();
    /* -1 because buffer full leaves one slot empty */
    return tx_buffer.size - used - 1;
}

/**
 * @brief Check if TX transmission is in progress.
 *
 * @return true if transmission is ongoing, false otherwise.
 */
bool UART_tx_busy(void)
{
    return UART_hw_tx_busy();
}

/**
 * @brief Set RX callback to be triggered when threshold bytes are available.
 *
 * @param callback Function to call when threshold is reached (NULL to disable)
 * @param threshold Number of bytes that must be available to trigger callback
 */
void UART_set_rx_callback(
    uart_rx_callback_t callback,
    uint32_t const threshold
) {
    rx_callback = callback;
    rx_threshold = threshold;

    /* Check if callback should be triggered immediately */
    if (rx_callback && rx_buffer_available() >= rx_threshold) {
        rx_callback(rx_buffer_available());
    }
}
