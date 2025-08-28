/**
 * @file fixed_point.c
 * @brief Fixed-point arithmetic functions
 *
 * This file implements fixed-point arithmetic functions unsuited for inlining
 * in fixed_point.h.
 */
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "fixed_point.h"

char *FIXED_to_string(
    FIXED_Q1616 const x,
    char *const buffer,
    size_t const buffer_size
)
{
    // Worst case: 1 minus sign, 5 integer digits, 1 decimal point,
    // 5 fractional digits, 1 null terminator == 13
    unsigned const buffer_min_size = 13;
    if (buffer == nullptr || buffer_size < buffer_min_size) {
        return nullptr;
    }

    // Get integer and fractional parts
    int16_t const integer_part = FIXED_get_integer_part(x);
    uint16_t const frac_part = FIXED_get_fractional_part(x);

    // Convert fractional part to decimal with five decimal places
    uint32_t const places = 100000;
    // Use 64-bit arithmetic to avoid overflow
    uint32_t decimal_frac =
        (((uint64_t)frac_part * places + FIXED_HALF) / FIXED_SCALE);

    // Format the string with all five decimal places first
    int result = snprintf(
        buffer, buffer_size, "%d.%05" PRIu32, integer_part, decimal_frac
    );

    if (result < 0 || (size_t)result >= buffer_size) {
        return nullptr;
    }

    // Find the decimal point and truncate trailing zeros
    char *decimal_point = strchr(buffer, '.');
    if (decimal_point != nullptr) {
        char *end = buffer + result - 1;

        // Remove trailing zeros, but keep at least one decimal place
        while (end > decimal_point + 1 && *end == '0') {
            *end = '\0';
            end--;
        }

        return buffer;
    }

    // Should be unreachable
    return nullptr;
}
