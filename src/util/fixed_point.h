/**
 * @file fixed_point.h
 * @brief Simple fixed-point arithmetic library for PSLab firmware
 *
 * This header provides basic fixed-point arithmetic operations for use in
 * embedded systems where floating-point operations are expensive or
 * unavailable.
 *
 * Uses Q16.16 format (16 bits integer, 16 bits fractional) by default.
 *
 * @author PSLab Team
 * @date 2025-07-18
 */

#ifndef PSLAB_FIXED_POINT_H
#define PSLAB_FIXED_POINT_H

#include <stdint.h>

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
typedef int32_t Fixed;

/**
 * @brief Number of fractional bits
 */
#define FIXED_FRAC_BITS 16

/**
 * @brief Fixed-point scale factor (2^16)
 */
#define FIXED_SCALE (1L << FIXED_FRAC_BITS)

/**
 * @brief Convert integer to fixed-point
 */
#define FIXED_FROM_INT(x) ((Fixed)((x) << FIXED_FRAC_BITS))

/**
 * @brief Convert fixed-point to integer (truncates)
 */
#define FIXED_TO_INT(x) ((int32_t)((x) >> FIXED_FRAC_BITS))

/**
 * @brief Fixed-point constants
 */
#define FIXED_ZERO FIXED_FROM_INT(0)
#define FIXED_ONE FIXED_FROM_INT(1)
#define FIXED_TWO FIXED_FROM_INT(2)
#define FIXED_HALF (FIXED_ONE >> 1)

/**
 * @brief Convert float to fixed-point (for constants and initialization)
 *
 * Note: This uses floating-point arithmetic and should only be used
 * for compile-time constants or initialization where FPU is available.
 */
#define FIXED_FROM_FLOAT(f) ((Fixed)((f) * FIXED_SCALE))

/**
 * @brief Convert fixed-point to float (for debugging/output)
 */
static inline float FIXED_TO_FLOAT(Fixed x)
{
    return (float)x / (float)FIXED_SCALE;
}

/**
 * @brief Fixed-point addition
 */
static inline Fixed fixed_add(Fixed a, Fixed b) { return a + b; }

/**
 * @brief Fixed-point subtraction
 */
static inline Fixed fixed_sub(Fixed a, Fixed b) { return a - b; }

/**
 * @brief Fixed-point multiplication
 *
 * Uses 64-bit intermediate to avoid overflow, then scales back down.
 */
static inline Fixed fixed_mul(Fixed a, Fixed b)
{
    int64_t temp = (int64_t)a * (int64_t)b;
    return (Fixed)(temp >> FIXED_FRAC_BITS);
}

/**
 * @brief Fixed-point division
 *
 * Uses 64-bit intermediate to maintain precision.
 */
static inline Fixed fixed_div(Fixed a, Fixed b)
{
    if (b == 0) {
        return (a >= 0) ? INT32_MAX
                        : INT32_MIN; // Return max/min on divide by zero
    }
    int64_t temp = ((int64_t)a << FIXED_FRAC_BITS) / (int64_t)b;
    return (Fixed)temp;
}

/**
 * @brief Fixed-point multiplication by integer
 */
static inline Fixed fixed_mul_int(Fixed a, int32_t b) { return a * b; }

/**
 * @brief Fixed-point division by integer
 */
static inline Fixed fixed_div_int(Fixed a, int32_t b)
{
    if (b == 0) {
        return (a >= 0) ? INT32_MAX : INT32_MIN;
    }
    return a / b;
}

/**
 * @brief Create fixed-point from integer numerator and denominator
 *
 * Equivalent to: FIXED_FROM_INT(num) / FIXED_FROM_INT(den)
 * But more efficient for cases where both are integers.
 */
static inline Fixed fixed_from_fraction(int32_t numerator, int32_t denominator)
{
    if (denominator == 0) {
        return (numerator >= 0) ? INT32_MAX : INT32_MIN;
    }
    return fixed_div(FIXED_FROM_INT(numerator), FIXED_FROM_INT(denominator));
}

/**
 * @brief Convert millivolts (integer) to fixed-point volts
 */
static inline Fixed fixed_from_millivolts(uint32_t millivolts)
{
    return fixed_from_fraction((int32_t)millivolts, 1000);
}

/**
 * @brief Convert fixed-point volts to millivolts (integer)
 */
static inline uint32_t fixed_to_millivolts(Fixed volts)
{
    Fixed mv = fixed_mul_int(volts, 1000);
    int32_t result = FIXED_TO_INT(mv);
    return (result >= 0) ? (uint32_t)result : 0;
}

/**
 * @brief Get fractional part as integer (0-65535 for Q16.16)
 */
static inline uint16_t fixed_get_fractional_part(Fixed x)
{
    return (uint16_t)(x & 0xFFFF);
}

/**
 * @brief Get integer part
 */
static inline int16_t fixed_get_integer_part(Fixed x)
{
    return (int16_t)(x >> FIXED_FRAC_BITS);
}

#ifdef __cplusplus
}
#endif

#endif // PSLAB_FIXED_POINT_H
