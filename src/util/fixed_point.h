/**
 * @file fixed_point.h
 * @brief Simple fixed-point arithmetic library for PSLab firmware
 *
 * This header provides basic fixed-point arithmetic operations for use in
 * low-speed instruments to abstract away ADC details.
 *
 * @note Helper functions use 64-bit integers for intermediate calculations
 * @note Helper functions round to nearest
 * @note Helper functions saturate in case of overflow
 *
 * @author PSLab Team
 * @date 2025-07-18
 */

#ifndef PSLAB_FIXED_POINT_H
#define PSLAB_FIXED_POINT_H

#include <stdint.h>
#include <stdlib.h>

#include "util/logging.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Fixed-point data type (Q16.16 format)
 *
 * 32-bit signed integer representing a fixed-point number:
 * - Bits 31-16: Integer part (signed)
 * - Bits 15-0:  Fractional part
 *
 * Range: -32768.0 to +32767.99998... with precision ~0.000015
 */
typedef int32_t FIXED_Q1616;

/**
 * @brief Number of fractional bits
 */
enum { FIXED_FRAC_BITS = 16 };

/**
 * @brief Fixed-point scale factor (2^16)
 */
enum { FIXED_SCALE = (1L << FIXED_FRAC_BITS) };

/**
 * @brief Convert integer to fixed-point
 *
 * @note This macro version is for compile-time constants only. Use
 *       FIXED_from_int for runtime conversion.
 * @note Input must be within the range of FIXED_MIN_INT to FIXED_MAX_INT to
 *       avoid overflow
 */
#define FIXED_FROM_INT(x) ((FIXED_Q1616)((int32_t)(x) * FIXED_SCALE))
// Note: Input is cast to int32_t rather than int64_t to intentionally trigger
//       compiler warnings on out-of-range inputs

/**
 * @brief Convert fixed-point to integer (truncates towards zero)
 */
#define FIXED_TO_INT(x) ((int32_t)((x) / FIXED_SCALE))

/**
 * @brief Fixed-point constants
 */
enum {
    FIXED_ZERO = FIXED_FROM_INT(0),
    FIXED_ONE = FIXED_FROM_INT(1),
    FIXED_TWO = FIXED_FROM_INT(2),
    FIXED_HALF = (FIXED_ONE >> 1),
    /* Smallest fractional increment (1/65536) */
    FIXED_EPSILON = ((FIXED_Q1616)1)
};

/**
 * @brief Fixed-point limits
 */
#define FIXED_MAX (FIXED_Q1616) INT32_MAX
#define FIXED_MIN (FIXED_Q1616) INT32_MIN
/**
 * @brief Integer part limits
 */
#define FIXED_MAX_INT INT16_MAX
#define FIXED_MIN_INT INT16_MIN
/**
 * @brief Representable floating-point limits
 */
#define FIXED_MAX_FLOAT ((float)FIXED_MAX / (float)FIXED_SCALE)
#define FIXED_MIN_FLOAT ((float)FIXED_MIN / (float)FIXED_SCALE)

/**
 * @brief Convert float to fixed-point (for constants and initialization)
 *
 * @note This uses floating-point arithmetic and should only be used
 *       for compile-time constants
 * @note Input must be within the range of FIXED_MIN_FLOAT to FIXED_MAX_FLOAT
 */
#define FIXED_FROM_FLOAT(f) ((FIXED_Q1616)((f) * FIXED_SCALE))

/**
 * @brief Convert fixed-point to float (for debug output)
 */
#define FIXED_TO_FLOAT(x) ((float)(x) / (float)FIXED_SCALE)

/**
 * @brief Convert integer to fixed-point with saturation
 *
 * @note Use this for runtime conversions
 */
static inline FIXED_Q1616 FIXED_from_int(int32_t x)
{
    int64_t temp = (int64_t)x * FIXED_SCALE;

    if (temp > INT32_MAX) {
        return INT32_MAX;
    }

    if (temp < INT32_MIN) {
        return INT32_MIN;
    }

    return (FIXED_Q1616)temp;
}

/**
 * @brief Fixed-point addition
 *
 * @param a First term
 * @param b Second term
 * @return Sum of a and b
 */
static inline FIXED_Q1616 FIXED_add(FIXED_Q1616 a, FIXED_Q1616 b)
{
    int64_t sum = (int64_t)a + (int64_t)b;

    if (sum > INT32_MAX) {
        return INT32_MAX;
    }

    if (sum < INT32_MIN) {
        return INT32_MIN;
    }

    return (FIXED_Q1616)sum;
}

/**
 * @brief Fixed-point subtraction
 *
 * @param a First term
 * @param b Second term
 * @return Difference of a and b
 */
static inline FIXED_Q1616 FIXED_sub(FIXED_Q1616 a, FIXED_Q1616 b)
{
    int64_t diff = (int64_t)a - (int64_t)b;

    if (diff > INT32_MAX) {
        return INT32_MAX;
    }

    if (diff < INT32_MIN) {
        return INT32_MIN;
    }

    return (FIXED_Q1616)diff;
}

/**
 * @brief Fixed-point multiplication
 *
 * @param a First factor
 * @param b Second factor
 * @return Product of a and b
 */
static inline FIXED_Q1616 FIXED_mul(FIXED_Q1616 a, FIXED_Q1616 b)
{
    int64_t prod = (int64_t)a * (int64_t)b;
    // Don't rely on implementation-defined right-shift of negative numbers
    prod = prod + (prod >= 0 ? FIXED_HALF : -FIXED_HALF);
    prod /= (int64_t)FIXED_SCALE;

    if (prod > INT32_MAX) {
        return INT32_MAX;
    }

    if (prod < INT32_MIN) {
        return INT32_MIN;
    }

    return (FIXED_Q1616)prod;
}

/**
 * @brief Fixed-point division
 *
 * @param a Dividend
 * @param b Divisor
 * @return Quotient of a and b
 *
 * @note Division by zero returns INT32_MAX (positive or zero-valued dividend)
 *       or INT32_MIN (negative divisor)
 */
static inline FIXED_Q1616 FIXED_div(FIXED_Q1616 a, FIXED_Q1616 b)
{
    if (b == 0) {
        LOG_DEBUG("Division by zero");
        return (a >= 0) ? INT32_MAX : INT32_MIN;
    }

    int64_t quot = (int64_t)a * FIXED_SCALE;
    int64_t half = a >= 0 ? llabs(b) / 2 : -llabs(b) / 2;
    quot = (quot + half) / (int64_t)b;

    if (quot > INT32_MAX) {
        return INT32_MAX;
    }

    if (quot < INT32_MIN) {
        return INT32_MIN;
    }

    return (FIXED_Q1616)quot;
}

/**
 * @brief Create fixed-point from integer numerator and denominator
 *
 * @param a Numerator
 * @param b Denominator
 * @return Fixed-point representation of the fraction
 *
 * @note Result rounds to nearest
 * @note Zero-valued denominator returns INT32_MAX (positive or zero-valued
 *       numerator) or INT32_MIN (negative numerator)
 */
static inline FIXED_Q1616 FIXED_from_fraction(int32_t a, int32_t b)
{
    return FIXED_div((FIXED_Q1616)(a), (FIXED_Q1616)(b));
}

/**
 * @brief Get integer part
 */
static inline int16_t FIXED_get_integer_part(FIXED_Q1616 x)
{
    return (int16_t)(x / FIXED_SCALE);
}

/**
 * @brief Get fractional part as integer (0-65535 for Q16.16)
 */
static inline uint16_t FIXED_get_fractional_part(FIXED_Q1616 x)
{
    int16_t int_part = FIXED_get_integer_part(x);
    FIXED_Q1616 frac_part = x - FIXED_from_int(int_part);
    return (uint16_t)(x >= 0 ? frac_part : -frac_part);
}

/**
 * @brief Convert fixed-point to string representation for debug output
 *
 * @param x Fixed-point value to convert
 * @param buffer Buffer to store the string (must be at least 13 chars)
 * @param buffer_size Size of the buffer
 * @return Pointer to the buffer on success, nullptr on error
 *
 * @note Output format is "-32768.0"
 * @note Fractional part is displayed with at least one and up to five decimal
 *       places
 */
char *FIXED_to_string(FIXED_Q1616 x, char *buffer, size_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif // PSLAB_FIXED_POINT_H
