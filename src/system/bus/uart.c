/**
 * @file uart.c
 * @brief Hardware-independent UART (Universal Asynchronous Receiver/Transmitter) implementation
 *
 * This module provides the hardware-independent layer of the UART driver.
 * It manages circular buffers for transmission and reception, handles callbacks,
 * and provides the public API exposed through uart.h.
 *
 * Features:
 * - Support for multiple UART bus instances
 * - Non-blocking read/write operations with circular buffers
 * - Configurable RX callback for protocol implementations
 * - Buffer status inquiry functions
 * - Circular buffer implementation with automatic wrap-around
 *
 * This implementation relies on hardware-specific functions defined in
 * src/system/h563xx/uart_ll.c (or equivalent for other platforms).
 *
 * @author Alexander Bessman
 * @date 2025-06-28
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "bus.h"
#include "uart_ll.h"
#include "uart.h"

/**
 * @brief UART bus handle structure
 */
struct uart_handle_t {
    uart_bus_t bus_id;
    circular_buffer_t *rx_buffer;
    circular_buffer_t *tx_buffer;
    uint32_t volatile rx_dma_head;
    uart_rx_callback_t rx_callback;
    uint32_t rx_threshold;
    bool initialized;
};

/* Global array to keep track of active UART handles */
static uart_handle_t *active_handles[UART_BUS_COUNT] = {NULL};

/**
 * @brief Get the number of available UART bus instances.
 *
 * @return Number of UART buses supported by this platform
 */
size_t UART_get_bus_count(void)
{
    return UART_BUS_COUNT;
}

/**
 * @brief Get UART handle from bus instance
 *
 * @param bus UART bus instance
 * @return Pointer to handle or NULL if not found
 */
static uart_handle_t *get_handle_from_bus(uart_bus_t bus)
{
    if (bus >= UART_BUS_COUNT) {
        return NULL;
    }
    return active_handles[bus];
}

/**
 * @brief Get number of bytes available in RX buffer.
 */
static uint32_t rx_buffer_available(uart_handle_t *handle)
{
    if (!handle || !handle->initialized) {
        return 0;
    }

    /* Get current DMA write position */
    uint32_t dma_pos = UART_LL_get_dma_position(handle->bus_id);
    uint32_t dma_head = dma_pos;

    /* Update our cached head position */
    handle->rx_dma_head = dma_head;

    /* Update the circular buffer's head position based on DMA */
    handle->rx_buffer->head = dma_head;

    /* Calculate available bytes using common function */
    return circular_buffer_available(handle->rx_buffer);
}

/**
 * @brief Check if RX callback condition is met and call if needed
 * 
 * @return true if condition was met and callback called, false otherwise
 */
static bool check_rx_callback(uart_handle_t *handle)
{
    if (!handle || !handle->initialized) {
        return false;
    }

    if (!handle->rx_callback) {
        return false;
    }

    if (rx_buffer_available(handle) < handle->rx_threshold) {
        return false;
    }

    handle->rx_callback(handle, rx_buffer_available(handle));
    return true;
}

/**
 * @brief Callback invoked by hardware layer on UART idle line detection
 *
 * @param bus UART bus instance
 * @param dma_pos Current DMA position
 */
static void idle_callback(uart_bus_t bus, uint32_t dma_pos)
{
    uart_handle_t *handle = get_handle_from_bus(bus);
    if (!handle || !handle->initialized) {
        return;
    }

    /* Update our cached DMA position */
    handle->rx_dma_head = dma_pos;
    handle->rx_buffer->head = dma_pos;

    /* Check for callbacks */
    check_rx_callback(handle);
}

/**
 * @brief Callback invoked by hardware layer when RX DMA reaches end of buffer
 *
 * @param bus UART bus instance
 */
static void rx_complete_callback(uart_bus_t bus)
{
    uart_handle_t *handle = get_handle_from_bus(bus);
    if (!handle || !handle->initialized) {
        return;
    }

    /* Hardware layer restarts DMA */
    /* Reset DMA position */
    handle->rx_dma_head = 0;
    handle->rx_buffer->head = 0;

    /* Check if we should run application RX callback now */
    check_rx_callback(handle);
}

/**
 * @brief Get number of bytes available in TX buffer.
 */
static uint32_t tx_buffer_available(uart_handle_t *handle)
{
    if (!handle || !handle->initialized) {
        return 0;
    }
    return circular_buffer_available(handle->tx_buffer);
}

/**
 * @brief Start transmission if not already in progress.
 */
static void start_transmission(uart_handle_t *handle)
{
    if (!handle || !handle->initialized || 
        UART_LL_tx_busy(handle->bus_id) || 
        circular_buffer_is_empty(handle->tx_buffer)) {
        return;
    }

    /* Calculate how many contiguous bytes we can send */
    uint32_t available = tx_buffer_available(handle);
    uint32_t contiguous_bytes;

    if (handle->tx_buffer->tail <= handle->tx_buffer->head) {
        contiguous_bytes = handle->tx_buffer->head - handle->tx_buffer->tail;
    } else {
        contiguous_bytes = handle->tx_buffer->size - handle->tx_buffer->tail;
    }

    /* Limit DMA transfer size to available bytes */
    if (contiguous_bytes > available) {
        contiguous_bytes = available;
    }

    /* Start hardware TX DMA */
    UART_LL_start_dma_tx(
        handle->bus_id, 
        &handle->tx_buffer->buffer[handle->tx_buffer->tail], 
        contiguous_bytes
    );
}

/**
 * @brief Callback invoked by hardware layer when TX DMA reaches end of buffer
 *
 * @param bus UART bus instance
 * @param bytes_transferred Number of bytes transferred
 */
static void tx_complete_callback(uart_bus_t bus, uint32_t bytes_transferred)
{
    uart_handle_t *handle = get_handle_from_bus(bus);
    if (!handle || !handle->initialized) {
        return;
    }

    /* Update TX buffer tail with the number of bytes that were sent */
    handle->tx_buffer->tail = (
        (handle->tx_buffer->tail + bytes_transferred) % handle->tx_buffer->size
    );

    /* Try to start another transmission if there's more data */
    start_transmission(handle);
}

/**
 * @brief Initialize the UART peripheral.
 *
 * @param bus UART bus instance to initialize (0-based index)
 * @param rx_buffer Pointer to pre-allocated RX circular buffer
 * @param tx_buffer Pointer to pre-allocated TX circular buffer
 * @return Pointer to UART handle on success, NULL on failure
 */
uart_handle_t *UART_init(
    size_t bus, 
    circular_buffer_t *rx_buffer,
    circular_buffer_t *tx_buffer
) {
    if (!rx_buffer || !tx_buffer || bus >= UART_BUS_COUNT) {
        return NULL;
    }

    uart_bus_t bus_id = (uart_bus_t)bus;

    /* Check if bus is already initialized */
    if (active_handles[bus_id] != NULL) {
        return NULL;
    }

    /* Allocate handle */
    uart_handle_t *handle = malloc(sizeof(uart_handle_t));
    if (!handle) {
        return NULL;
    }

    /* Initialize handle */
    handle->bus_id = bus_id;
    handle->rx_buffer = rx_buffer;
    handle->tx_buffer = tx_buffer;
    handle->rx_dma_head = 0;
    handle->rx_callback = NULL;
    handle->rx_threshold = 0;
    handle->initialized = false;

    /* Initialize hardware layer */
    UART_LL_init(bus_id, rx_buffer->buffer, rx_buffer->size);
    UART_LL_set_idle_callback(bus_id, idle_callback);
    UART_LL_set_rx_complete_callback(bus_id, rx_complete_callback);
    UART_LL_set_tx_complete_callback(bus_id, tx_complete_callback);

    /* Mark as initialized and register in active handles */
    handle->initialized = true;
    active_handles[bus_id] = handle;

    return handle;
}

/**
 * @brief Deinitialize the UART peripheral.
 *
 * @param handle Pointer to UART handle structure
 */
void UART_deinit(uart_handle_t *handle)
{
    if (!handle || !handle->initialized || handle->bus_id >= UART_BUS_COUNT) {
        return;
    }

    /* Deinitialize hardware layer */
    UART_LL_deinit(handle->bus_id);

    /* Clear callbacks */
    UART_LL_set_idle_callback(handle->bus_id, NULL);
    UART_LL_set_rx_complete_callback(handle->bus_id, NULL);
    UART_LL_set_tx_complete_callback(handle->bus_id, NULL);

    /* Remove from active handles */
    active_handles[handle->bus_id] = NULL;

    /* Mark as uninitialized */
    handle->initialized = false;

    /* Free the handle */
    free(handle);
}

/**
 * @brief Write data to the UART interface.
 *
 * This function queues bytes for transmission using a circular buffer.
 * Data is sent via interrupt-driven transmission.
 *
 * @param handle     Pointer to UART handle structure.
 * @param txbuf      Pointer to the data buffer to be transmitted.
 * @param sz         Number of bytes to write from the buffer.
 *
 * @return Number of bytes actually written.
 */
uint32_t UART_write(uart_handle_t *handle, uint8_t const *txbuf, uint32_t const sz)
{
    if (!handle || !handle->initialized || txbuf == NULL || sz == 0) {
        return 0;
    }

    /* Queue bytes for transmission */
    uint32_t bytes_written = circular_buffer_write(handle->tx_buffer, txbuf, sz);

    /* Start transmission if not already in progress */
    start_transmission(handle);

    return bytes_written;
}

/**
 * @brief Read data from the UART interface.
 *
 * This function reads bytes from the receive circular buffer.
 * Returns the number of bytes actually read.
 *
 * @param handle     Pointer to UART handle structure.
 * @param rxbuf      Pointer to allocated memory into which to read data.
 * @param sz         Number of bytes to read into the buffer.
 *
 * @return Number of bytes actually read.
 */
uint32_t UART_read(uart_handle_t *handle, uint8_t *const rxbuf, uint32_t const sz)
{
    if (!handle || !handle->initialized || rxbuf == NULL || sz == 0) {
        return 0;
    }

    uint32_t available = rx_buffer_available(handle);
    uint32_t to_read = sz > available ? available : sz;

    /* Read from circular buffer using common function */
    return circular_buffer_read(handle->rx_buffer, rxbuf, to_read);
}

/**
 * @brief Check if data is available to read from the UART receive buffer.
 *
 * This function returns true if there is data available in the receive buffer,
 * indicating that there is unread data available to be received.
 *
 * @param handle Pointer to UART handle structure.
 * @return true if at least one byte is available to be read, false otherwise.
 */
bool UART_rx_ready(uart_handle_t *handle)
{
    return rx_buffer_available(handle) > 0;
}

/**
 * @brief Get number of bytes available in receive buffer.
 *
 * This function returns the number of bytes currently available
 * to read from the receive buffer.
 *
 * @param handle Pointer to UART handle structure.
 * @return Number of bytes available to read.
 */
uint32_t UART_rx_available(uart_handle_t *handle)
{
    return rx_buffer_available(handle);
}

/**
 * @brief Get TX buffer free space.
 *
 * @param handle Pointer to UART handle structure.
 * @return Number of bytes that can still be written to TX buffer.
 */
uint32_t UART_tx_free_space(uart_handle_t *handle)
{
    if (!handle || !handle->initialized) {
        return 0;
    }

    uint32_t used = tx_buffer_available(handle);
    /* -1 because buffer full leaves one slot empty */
    return handle->tx_buffer->size - used - 1;
}

/**
 * @brief Check if TX transmission is in progress.
 *
 * @param handle Pointer to UART handle structure.
 * @return true if transmission is ongoing, false otherwise.
 */
bool UART_tx_busy(uart_handle_t *handle)
{
    if (!handle || !handle->initialized) {
        return false;
    }
    return UART_LL_tx_busy(handle->bus_id);
}

/**
 * @brief Set RX callback to be triggered when threshold bytes are available.
 *
 * @param handle Pointer to UART handle structure.
 * @param callback Function to call when threshold is reached (NULL to disable)
 * @param threshold Number of bytes that must be available to trigger callback
 */
void UART_set_rx_callback(
    uart_handle_t *handle,
    uart_rx_callback_t callback,
    uint32_t const threshold
) {
    if (!handle || !handle->initialized) {
        return;
    }

    handle->rx_callback = callback;
    handle->rx_threshold = threshold;

    /* Check if callback should be triggered immediately */
    if (handle->rx_callback && rx_buffer_available(handle) >= handle->rx_threshold) {
        handle->rx_callback(handle, rx_buffer_available(handle));
    }
}
