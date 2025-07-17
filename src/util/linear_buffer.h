/**
 * @file linear_buffer.h
 * @brief Linear buffer implementation
 *
 * This module provides a linear buffer implementation for storing and
 * processing data.
 *
 * @author Tejas Garg
 * @date 2025-07-18
 */

#ifndef LINEAR_BUFFER_H
#define LINEAR_BUFFER_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint32_t *buffer; // Pointer to the buffer data
    uint32_t size; // Size of the buffer
} LinearBuffer;

/**
 * @brief Initializes a linear buffer.
 *
 * @param buf Pointer to the linear buffer structure.
 * @param data Pointer to the buffer data array.
 * @param size Size of the buffer.
 */
void linear_buffer_init(
    LinearBuffer *linear_buffer,
    uint32_t *buffer,
    uint32_t size
);

/**
 * @brief Reads data from the linear buffer.
 *
 * @param buf Pointer to the linear buffer structure.
 * @param data Pointer to the buffer to store the read data.
 * @param size Size of the data to read.
 * @return Number of bytes read.
 */
uint32_t linear_buffer_read(
    LinearBuffer *linear_buffer,
    uint32_t *data,
    uint32_t size
);

#endif /* LINEAR_BUFFER_H */