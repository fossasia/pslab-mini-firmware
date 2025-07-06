/**
 * @file bus.c
 * @brief Common utilities for bus interfaces (UART, USB, etc.)
 *
 * This module provides shared functionality for various bus interfaces,
 * including circular buffer implementations and utility functions.
 */

#include <stdbool.h>
#include <stdint.h>

#include "bus.h"

/**
 * @brief Initialize a circular buffer
 *
 * @param cb Pointer to circular buffer structure
 * @param buffer Memory area to use for the buffer
 * @param size Size of the buffer (should be a power of 2)
 */
void circular_buffer_init(BUS_CircBuffer *cb, uint8_t *buffer, uint32_t size)
{
    cb->buffer = buffer;
    cb->head = 0;
    cb->tail = 0;
    cb->size = size;
}

/**
 * @brief Check if circular buffer is empty
 *
 * @param cb Pointer to circular buffer structure
 * @return true if buffer is empty, false otherwise
 */
bool circular_buffer_is_empty(BUS_CircBuffer *cb)
{
    return cb->head == cb->tail;
}

/**
 * @brief Check if circular buffer is full
 *
 * @param cb Pointer to circular buffer structure
 * @return true if buffer is full, false otherwise
 */
bool circular_buffer_is_full(BUS_CircBuffer *cb)
{
    return ((cb->head + 1) % cb->size) == cb->tail;
}

/**
 * @brief Get number of bytes available in circular buffer
 *
 * @param cb Pointer to circular buffer structure
 * @return Number of bytes available
 */
uint32_t circular_buffer_available(BUS_CircBuffer *cb)
{
    if (cb->head >= cb->tail) {
        return cb->head - cb->tail;
    }
    return cb->size - cb->tail + cb->head;
}

/**
 * @brief Put a byte into circular buffer
 *
 * @param cb Pointer to circular buffer structure
 * @param data Byte to put into buffer
 * @return true if successful, false if buffer is full
 */
bool circular_buffer_put(BUS_CircBuffer *cb, uint8_t data)
{
    if (circular_buffer_is_full(cb)) {
        return false;
    }

    cb->buffer[cb->head] = data;
    cb->head = (cb->head + 1) % cb->size;
    return true;
}

/**
 * @brief Get a byte from circular buffer
 *
 * @param cb Pointer to circular buffer structure
 * @param data Pointer to store the read byte
 * @return true if successful, false if buffer is empty
 */
bool circular_buffer_get(BUS_CircBuffer *cb, uint8_t *data)
{
    if (circular_buffer_is_empty(cb)) {
        return false;
    }

    *data = cb->buffer[cb->tail];
    cb->tail = (cb->tail + 1) % cb->size;
    return true;
}

/**
 * @brief Reset (empty) a circular buffer
 *
 * @param cb Pointer to circular buffer structure
 */
void circular_buffer_reset(BUS_CircBuffer *cb) { cb->head = cb->tail = 0; }

/**
 * @brief Write multiple bytes to circular buffer
 *
 * @param cb Pointer to circular buffer structure
 * @param data Pointer to data to write
 * @param len Number of bytes to write
 * @return Number of bytes actually written
 */
uint32_t circular_buffer_write(
    BUS_CircBuffer *cb,
    uint8_t const *data,
    uint32_t len
)
{
    uint32_t bytes_written = 0;

    while (bytes_written < len && !circular_buffer_is_full(cb)) {
        if (circular_buffer_put(cb, data[bytes_written])) {
            bytes_written++;
        } else {
            break;
        }
    }

    return bytes_written;
}

/**
 * @brief Read multiple bytes from circular buffer
 *
 * @param cb Pointer to circular buffer structure
 * @param data Pointer to buffer for read data
 * @param len Maximum number of bytes to read
 * @return Number of bytes actually read
 */
uint32_t circular_buffer_read(
    BUS_CircBuffer *cb,
    uint8_t *data,
    uint32_t len
)
{
    uint32_t bytes_read = 0;

    while (bytes_read < len && !circular_buffer_is_empty(cb)) {
        if (circular_buffer_get(cb, &data[bytes_read])) {
            bytes_read++;
        } else {
            break;
        }
    }

    return bytes_read;
}
