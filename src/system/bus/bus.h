/**
 * @file bus.h
 * @brief Common utilities for bus interfaces (UART, USB, etc.)
 *
 * This module provides shared functionality for various bus interfaces,
 * including circular buffer implementations and utility functions.
 */

#ifndef BUS_COMMON_H
#define BUS_COMMON_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Circular buffer structure
 *
 * This structure manages a circular buffer with head and tail pointers.
 */
typedef struct {
    uint8_t *buffer;
    uint32_t volatile head;
    uint32_t volatile tail;
    uint32_t size;
} BUS_CircBuffer;

/**
 * @brief Initialize a circular buffer
 *
 * @param cb Pointer to circular buffer structure
 * @param buffer Memory area to use for the buffer
 * @param size Size of the buffer (should be a power of 2)
 */
void circular_buffer_init(
    BUS_CircBuffer *cb,
    uint8_t *buffer,
    uint32_t size
);

/**
 * @brief Check if circular buffer is empty
 *
 * @param cb Pointer to circular buffer structure
 * @return true if buffer is empty, false otherwise
 */
bool circular_buffer_is_empty(BUS_CircBuffer *cb);

/**
 * @brief Check if circular buffer is full
 *
 * @param cb Pointer to circular buffer structure
 * @return true if buffer is full, false otherwise
 */
bool circular_buffer_is_full(BUS_CircBuffer *cb);

/**
 * @brief Get number of bytes available in circular buffer
 *
 * @param cb Pointer to circular buffer structure
 * @return Number of bytes available
 */
uint32_t circular_buffer_available(BUS_CircBuffer *cb);

/**
 * @brief Put a byte into circular buffer
 *
 * @param cb Pointer to circular buffer structure
 * @param data Byte to put into buffer
 * @return true if successful, false if buffer is full
 */
bool circular_buffer_put(BUS_CircBuffer *cb, uint8_t data);

/**
 * @brief Get a byte from circular buffer
 *
 * @param cb Pointer to circular buffer structure
 * @param data Pointer to store the read byte
 * @return true if successful, false if buffer is empty
 */
bool circular_buffer_get(BUS_CircBuffer *cb, uint8_t *data);

/**
 * @brief Reset (empty) a circular buffer
 *
 * @param cb Pointer to circular buffer structure
 */
void circular_buffer_reset(BUS_CircBuffer *cb);

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
);

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
);

#ifdef __cplusplus
}
#endif

#endif /* BUS_COMMON_H */
