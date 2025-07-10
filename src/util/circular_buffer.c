/**
 * @file circular_buffer.c
 * @brief Circular buffer implementation for PSLab mini firmware
 *
 * This module provides a circular buffer implementation for the PSLab firmware,
 * allowing for efficient data storage and retrieval in a fixed-size buffer.
 */

#include <stdbool.h>
#include <stdint.h>

#include "util.h"

/**
 * @brief Initialize a circular buffer
 *
 * @param cb Pointer to circular buffer structure
 * @param buffer Memory area to use for the buffer
 * @param size Size of the buffer (must be a power of 2)
 */
void circular_buffer_init(CircularBuffer *cb, uint8_t *buffer, uint32_t size)
{
    // Require power of two size
    if ((size == 0) || (size & (size - 1)) != 0) {
        cb->buffer = nullptr; // Invalid size
        cb->head = 0;
        cb->tail = 0;
        cb->size = 0;
        cb->mask = 0;
        return;
    }
    cb->buffer = buffer;
    cb->head = 0;
    cb->tail = 0;
    cb->size = size;
    cb->mask = size - 1; // Mask for bitwise operations
}

/**
 * @brief Check if circular buffer is empty
 *
 * @param cb Pointer to circular buffer structure
 * @return true if buffer is empty, false otherwise
 */
bool circular_buffer_is_empty(CircularBuffer *cb)
{
    return cb->head == cb->tail;
}

/**
 * @brief Check if circular buffer is full
 *
 * @param cb Pointer to circular buffer structure
 * @return true if buffer is full, false otherwise
 */
bool circular_buffer_is_full(CircularBuffer *cb)
{
    return ((cb->head + 1) & cb->mask) == cb->tail;
}

/**
 * @brief Get number of bytes available in circular buffer
 *
 * @param cb Pointer to circular buffer structure
 * @return Number of bytes available
 */
uint32_t circular_buffer_available(CircularBuffer *cb)
{
    return (cb->head - cb->tail) & cb->mask;
}

/**
 * @brief Put a byte into circular buffer
 *
 * @param cb Pointer to circular buffer structure
 * @param data Byte to put into buffer
 * @return true if successful, false if buffer is full
 */
bool circular_buffer_put(CircularBuffer *cb, uint8_t data)
{
    if (circular_buffer_is_full(cb)) {
        return false;
    }

    cb->buffer[cb->head] = data;
    cb->head = (cb->head + 1) & cb->mask;
    return true;
}

/**
 * @brief Get a byte from circular buffer
 *
 * @param cb Pointer to circular buffer structure
 * @param data Pointer to store the read byte
 * @return true if successful, false if buffer is empty
 */
bool circular_buffer_get(CircularBuffer *cb, uint8_t *data)
{
    if (circular_buffer_is_empty(cb)) {
        return false;
    }

    *data = cb->buffer[cb->tail];
    cb->tail = (cb->tail + 1) & cb->mask;
    return true;
}

/**
 * @brief Reset (empty) a circular buffer
 *
 * @param cb Pointer to circular buffer structure
 */
void circular_buffer_reset(CircularBuffer *cb) { cb->head = cb->tail = 0; }

/**
 * @brief Write multiple bytes to circular buffer
 *
 * @param cb Pointer to circular buffer structure
 * @param data Pointer to data to write
 * @param len Number of bytes to write
 * @return Number of bytes actually written
 */
uint32_t circular_buffer_write(
    CircularBuffer *cb,
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
uint32_t circular_buffer_read(CircularBuffer *cb, uint8_t *data, uint32_t len)
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

/**
 * @brief Get free space in circular buffer
 *
 * @param cb Pointer to circular buffer structure
 * @return Number of bytes free in the buffer
 */
uint32_t circular_buffer_free_space(CircularBuffer *cb)
{
    return (cb->size - 1) - ((cb->head - cb->tail) & cb->mask);
}
