/**
 * @file util.h
 * @brief Utility types and functions for PSLab mini firmware
 *
 * This library provides utility types and functions for the PSLab firmware.
 */

#ifndef PSLAB_UTIL_H
#define PSLAB_UTIL_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Circular buffer structure
 *
 * This structure manages a circular buffer with head and tail pointers.
 * Used throughout the codebase for buffering data in UART, USB, and logging.
 */
typedef struct {
    uint8_t *buffer;
    uint32_t volatile head;
    uint32_t volatile tail;
    uint32_t size;
    uint32_t mask; // mask for power-of-two optimization
} CircularBuffer;

/**
 * @brief Initialize a circular buffer
 *
 * @param cb Pointer to circular buffer structure
 * @param buffer Memory area to use for the buffer
 * @param size Size of the buffer (should be a power of 2)
 */
void circular_buffer_init(CircularBuffer *cb, uint8_t *buffer, uint32_t size);

/**
 * @brief Check if circular buffer is empty
 *
 * @param cb Pointer to circular buffer structure
 * @return true if buffer is empty, false otherwise
 */
bool circular_buffer_is_empty(CircularBuffer *cb);

/**
 * @brief Check if circular buffer is full
 *
 * @param cb Pointer to circular buffer structure
 * @return true if buffer is full, false otherwise
 */
bool circular_buffer_is_full(CircularBuffer *cb);

/**
 * @brief Get number of bytes available in circular buffer
 *
 * @param cb Pointer to circular buffer structure
 * @return Number of bytes available
 */
uint32_t circular_buffer_available(CircularBuffer *cb);

/**
 * @brief Put a byte into circular buffer
 *
 * @param cb Pointer to circular buffer structure
 * @param data Byte to put into buffer
 * @return true if successful, false if buffer is full
 */
bool circular_buffer_put(CircularBuffer *cb, uint8_t data);

/**
 * @brief Get a byte from circular buffer
 *
 * @param cb Pointer to circular buffer structure
 * @param data Pointer to store the read byte
 * @return true if successful, false if buffer is empty
 */
bool circular_buffer_get(CircularBuffer *cb, uint8_t *data);

/**
 * @brief Reset (empty) a circular buffer
 *
 * @param cb Pointer to circular buffer structure
 */
void circular_buffer_reset(CircularBuffer *cb);

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
);

/**
 * @brief Read multiple bytes from circular buffer
 *
 * @param cb Pointer to circular buffer structure
 * @param data Pointer to buffer for read data
 * @param len Maximum number of bytes to read
 * @return Number of bytes actually read
 */
uint32_t circular_buffer_read(CircularBuffer *cb, uint8_t *data, uint32_t len);

/**
 * @brief Get free space in circular buffer
 *
 * @param cb Pointer to circular buffer structure
 * @return Number of bytes free in the buffer
 */
uint32_t circular_buffer_free_space(CircularBuffer *cb);

#ifdef __cplusplus
}
#endif

#endif /* PSLAB_UTIL_H */
