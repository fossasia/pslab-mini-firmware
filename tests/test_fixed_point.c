/**
 * @file test_fixed_point.c
 * @brief Unit tests for fixed-point arithmetic library
 *
 * Tests focus on edge cases including overflow, underflow, division by zero,
 * rounding behavior, and boundary conditions.
 *
 * @author PSLab Team
 * @date 2025-08-26
 */

#include "unity.h"
#include "fixed_point.h"
#include <stdint.h>
#include <limits.h>
#include <string.h>

void setUp(void)
{
    // Set up any test fixtures here
}

void tearDown(void)
{
    // Clean up after each test
}

/**
 * Test basic conversion macros
 */
void test_FIXED_conversions(void)
{
    // Integer conversions
    TEST_ASSERT_EQUAL(0, FIXED_TO_INT(FIXED_FROM_INT(0)));
    TEST_ASSERT_EQUAL(1, FIXED_TO_INT(FIXED_FROM_INT(1)));
    TEST_ASSERT_EQUAL(-1, FIXED_TO_INT(FIXED_FROM_INT(-1)));
    TEST_ASSERT_EQUAL(100, FIXED_TO_INT(FIXED_FROM_INT(100)));
    TEST_ASSERT_EQUAL(-100, FIXED_TO_INT(FIXED_FROM_INT(-100)));

    // Maximum safe integer values for Q16.16
    TEST_ASSERT_EQUAL(32767, FIXED_TO_INT(FIXED_FROM_INT(32767)));
    TEST_ASSERT_EQUAL(-32768, FIXED_TO_INT(FIXED_FROM_INT(-32768)));

    // Float conversions (compile-time constants)
    TEST_ASSERT_EQUAL(FIXED_FROM_INT(0), FIXED_FROM_FLOAT(0.0f));
    TEST_ASSERT_EQUAL(FIXED_FROM_INT(1), FIXED_FROM_FLOAT(1.0f));
    TEST_ASSERT_EQUAL(FIXED_FROM_INT(-1), FIXED_FROM_FLOAT(-1.0f));

    // Test half value
    TEST_ASSERT_EQUAL(FIXED_HALF, FIXED_FROM_FLOAT(0.5f));
    TEST_ASSERT_EQUAL(0, FIXED_TO_INT(FIXED_HALF)); // Should truncate to 0
}

/**
 * Test fixed-point constants
 */
void test_FIXED_constants(void)
{
    TEST_ASSERT_EQUAL(0, FIXED_ZERO);
    TEST_ASSERT_EQUAL(FIXED_SCALE, FIXED_ONE);
    TEST_ASSERT_EQUAL(2 * FIXED_SCALE, FIXED_TWO);
    TEST_ASSERT_EQUAL(FIXED_SCALE / 2, FIXED_HALF);
    TEST_ASSERT_EQUAL(INT32_MAX, FIXED_MAX);
    TEST_ASSERT_EQUAL(INT32_MIN, FIXED_MIN);
}

/**
 * Test fractional and integer part extraction
 */
void test_FIXED_part_extraction(void)
{
    FIXED_Q1616 val = FIXED_FROM_FLOAT(123.456f);

    TEST_ASSERT_EQUAL(123, FIXED_get_integer_part(val));
    // 0.456 * 65536 ≈ 29884
    TEST_ASSERT_EQUAL(29884, FIXED_get_fractional_part(val));

    // Test with negative number
    FIXED_Q1616 neg_val = FIXED_FROM_FLOAT(-123.456f);
    TEST_ASSERT_EQUAL(-123, FIXED_get_integer_part(neg_val));
    // For negative numbers, fractional part should be the same as positive
    TEST_ASSERT_EQUAL(29884, FIXED_get_fractional_part(neg_val));

    // Test range limits
    TEST_ASSERT_EQUAL(FIXED_MAX_INT, FIXED_get_integer_part(FIXED_MAX));
    TEST_ASSERT_EQUAL(0xFFFF, FIXED_get_fractional_part(FIXED_MAX));
    TEST_ASSERT_EQUAL(FIXED_MIN_INT, FIXED_get_integer_part(FIXED_MIN));
    TEST_ASSERT_EQUAL(0, FIXED_get_fractional_part(FIXED_MIN));

    // Test edge cases
    TEST_ASSERT_EQUAL(0, FIXED_get_integer_part(FIXED_ZERO));
    TEST_ASSERT_EQUAL(0, FIXED_get_fractional_part(FIXED_ZERO));
    TEST_ASSERT_EQUAL(1, FIXED_get_integer_part(FIXED_ONE));
    TEST_ASSERT_EQUAL(0, FIXED_get_fractional_part(FIXED_ONE));
}

/**
 * Test addition with overflow/underflow
 */
void test_FIXED_add_overflow(void)
{
    // Normal addition
    TEST_ASSERT_EQUAL(FIXED_FROM_INT(3), FIXED_add(FIXED_FROM_INT(1), FIXED_FROM_INT(2)));
    TEST_ASSERT_EQUAL(FIXED_FROM_INT(-1), FIXED_add(FIXED_FROM_INT(1), FIXED_FROM_INT(-2)));

    // Positive overflow - should saturate to INT32_MAX
    FIXED_Q1616 large_pos = INT32_MAX - 1000;
    FIXED_Q1616 result = FIXED_add(large_pos, FIXED_FROM_INT(1));
    TEST_ASSERT_EQUAL(INT32_MAX, result);

    // Negative overflow - should saturate to INT32_MIN
    FIXED_Q1616 large_neg = INT32_MIN + 1000;
    result = FIXED_add(large_neg, FIXED_FROM_INT(-1));
    TEST_ASSERT_EQUAL(INT32_MIN, result);

    // Edge case: adding to maximum values
    TEST_ASSERT_EQUAL(INT32_MAX, FIXED_add(INT32_MAX, FIXED_ONE));
    TEST_ASSERT_EQUAL(INT32_MIN, FIXED_add(INT32_MIN, -FIXED_ONE));

    // Adding zero should not change value
    TEST_ASSERT_EQUAL(FIXED_FROM_INT(42), FIXED_add(FIXED_FROM_INT(42), FIXED_ZERO));
}

/**
 * Test subtraction with overflow/underflow
 */
void test_FIXED_sub_overflow(void)
{
    // Normal subtraction
    TEST_ASSERT_EQUAL(FIXED_FROM_INT(1), FIXED_sub(FIXED_FROM_INT(3), FIXED_FROM_INT(2)));
    TEST_ASSERT_EQUAL(FIXED_FROM_INT(-1), FIXED_sub(FIXED_FROM_INT(1), FIXED_FROM_INT(2)));

    // Positive overflow (subtracting large negative from positive)
    FIXED_Q1616 large_pos = INT32_MAX - 1000;
    FIXED_Q1616 large_neg = INT32_MIN + 1000;
    FIXED_Q1616 result = FIXED_sub(large_pos, large_neg);
    TEST_ASSERT_EQUAL(INT32_MAX, result);

    // Negative overflow (subtracting large positive from negative)
    result = FIXED_sub(large_neg, large_pos);
    TEST_ASSERT_EQUAL(INT32_MIN, result);

    // Edge cases
    TEST_ASSERT_EQUAL(INT32_MAX, FIXED_sub(INT32_MAX, -FIXED_ONE));
    TEST_ASSERT_EQUAL(INT32_MIN, FIXED_sub(INT32_MIN, FIXED_ONE));

    // Subtracting zero should not change value
    TEST_ASSERT_EQUAL(FIXED_FROM_INT(42), FIXED_sub(FIXED_FROM_INT(42), FIXED_ZERO));
}

/**
 * Test multiplication with overflow and rounding
 */
void test_FIXED_mul_overflow_and_rounding(void)
{
    // Normal multiplication
    TEST_ASSERT_EQUAL(FIXED_FROM_INT(6), FIXED_mul(FIXED_FROM_INT(2), FIXED_FROM_INT(3)));
    TEST_ASSERT_EQUAL(FIXED_FROM_INT(-6), FIXED_mul(FIXED_FROM_INT(-2), FIXED_FROM_INT(3)));
    TEST_ASSERT_EQUAL(FIXED_FROM_INT(6), FIXED_mul(FIXED_FROM_INT(-2), FIXED_FROM_INT(-3)));

    // Multiplication by one should not change value
    TEST_ASSERT_EQUAL(FIXED_FROM_INT(42), FIXED_mul(FIXED_FROM_INT(42), FIXED_ONE));

    // Multiplication by zero
    TEST_ASSERT_EQUAL(FIXED_ZERO, FIXED_mul(FIXED_FROM_INT(42), FIXED_ZERO));

    // Test with fractional values
    FIXED_Q1616 half = FIXED_HALF;
    TEST_ASSERT_EQUAL(FIXED_FROM_INT(1), FIXED_mul(FIXED_FROM_INT(2), half));
    TEST_ASSERT_EQUAL(FIXED_FROM_INT(21), FIXED_mul(FIXED_FROM_INT(42), half));

    // Test overflow - multiplying large values should saturate
    FIXED_Q1616 result = FIXED_mul(FIXED_MAX, FIXED_MAX);
    TEST_ASSERT_EQUAL(INT32_MAX, result);

    // Test negative overflow
    result = FIXED_mul(FIXED_MIN, FIXED_MAX);
    TEST_ASSERT_EQUAL(INT32_MIN, result);

    // Test rounding behavior - multiply by slightly more than 1
    FIXED_Q1616 slightly_more_than_one = FIXED_ONE + FIXED_EPSILON;
    result = FIXED_mul(FIXED_FROM_INT(10), slightly_more_than_one);
    // Should be very close to 10, allowing for the tiny increment
    TEST_ASSERT_TRUE(result > FIXED_FROM_INT(10));
    TEST_ASSERT_TRUE(result < FIXED_FROM_INT(11));

    // Test multiply FIXED_MIN by -1
    result = FIXED_mul(FIXED_MIN, -FIXED_ONE);
    TEST_ASSERT_EQUAL(INT32_MAX, result);
}

/**
 * Test division with overflow, underflow, and division by zero
 */
void test_FIXED_div_edge_cases(void)
{
    // Normal division
    TEST_ASSERT_EQUAL(FIXED_FROM_INT(2), FIXED_div(FIXED_FROM_INT(6), FIXED_FROM_INT(3)));
    TEST_ASSERT_EQUAL(FIXED_FROM_INT(-2), FIXED_div(FIXED_FROM_INT(-6), FIXED_FROM_INT(3)));
    TEST_ASSERT_EQUAL(FIXED_FROM_INT(2), FIXED_div(FIXED_FROM_INT(-6), FIXED_FROM_INT(-3)));

    // Division by one should not change value
    TEST_ASSERT_EQUAL(FIXED_FROM_INT(42), FIXED_div(FIXED_FROM_INT(42), FIXED_ONE));

    // Division by two
    TEST_ASSERT_EQUAL(FIXED_HALF, FIXED_div(FIXED_ONE, FIXED_TWO));
    TEST_ASSERT_EQUAL(FIXED_FROM_INT(21), FIXED_div(FIXED_FROM_INT(42), FIXED_TWO));

    // Division by zero - should return appropriate infinity
    TEST_ASSERT_EQUAL(INT32_MAX, FIXED_div(FIXED_FROM_INT(1), FIXED_ZERO));
    TEST_ASSERT_EQUAL(INT32_MIN, FIXED_div(FIXED_FROM_INT(-1), FIXED_ZERO));
    TEST_ASSERT_EQUAL(INT32_MAX, FIXED_div(FIXED_ZERO, FIXED_ZERO)); // 0/0 -> +inf

    // Division of FIXED_MIN by -1
    TEST_ASSERT_EQUAL(INT32_MAX, FIXED_div(FIXED_MIN, -FIXED_ONE));

    // Division resulting in overflow
    FIXED_Q1616 result = FIXED_div(INT32_MAX, FIXED_HALF);
    TEST_ASSERT_EQUAL(INT32_MAX, result);

    // Division resulting in underflow
    result = FIXED_div(INT32_MIN, FIXED_HALF);
    TEST_ASSERT_EQUAL(INT32_MIN, result);

    // Test rounding behavior
    FIXED_Q1616 one_third = FIXED_div(FIXED_ONE, FIXED_FROM_INT(3));
    FIXED_Q1616 three_thirds = FIXED_mul(one_third, FIXED_FROM_INT(3));
    // Should be close to 1 due to rounding
    TEST_ASSERT_TRUE(abs(three_thirds - FIXED_ONE) <= 2);
}

/**
 * Test FIXED_from_fraction function
 */
void test_FIXED_from_fraction(void)
{
    // Basic fractions
    TEST_ASSERT_EQUAL(FIXED_HALF, FIXED_from_fraction(1, 2));
    TEST_ASSERT_EQUAL(FIXED_ONE, FIXED_from_fraction(3, 3));
    TEST_ASSERT_EQUAL(FIXED_FROM_INT(2), FIXED_from_fraction(6, 3));

    // Negative fractions
    TEST_ASSERT_EQUAL(-FIXED_HALF, FIXED_from_fraction(-1, 2));
    TEST_ASSERT_EQUAL(-FIXED_HALF, FIXED_from_fraction(1, -2));
    TEST_ASSERT_EQUAL(FIXED_HALF, FIXED_from_fraction(-1, -2));

    // Division by zero
    TEST_ASSERT_EQUAL(INT32_MAX, FIXED_from_fraction(1, 0));
    TEST_ASSERT_EQUAL(INT32_MIN, FIXED_from_fraction(-1, 0));

    // Large fractions that might overflow in the division step
    FIXED_Q1616 result = FIXED_from_fraction(100000, 1);
    TEST_ASSERT_EQUAL(FIXED_MAX, result);

    result = FIXED_from_fraction(-100000, 1);
    TEST_ASSERT_EQUAL(FIXED_MIN, result);
}

/**
 * Test precision and accuracy
 */
void test_FIXED_precision(void)
{
    // Test that we can represent small fractions accurately
    FIXED_Q1616 small_frac = FIXED_FROM_FLOAT(0.0001f);
    TEST_ASSERT_TRUE(small_frac > 0);

    // Test precision near boundaries
    FIXED_Q1616 near_max = FIXED_FROM_FLOAT(32767.9f);
    TEST_ASSERT_TRUE(near_max < INT32_MAX);
    TEST_ASSERT_EQUAL(32767, FIXED_TO_INT(near_max));

    FIXED_Q1616 near_min = FIXED_FROM_FLOAT(-32767.9f);
    TEST_ASSERT_TRUE(near_min > INT32_MIN);
    TEST_ASSERT_EQUAL(-32767, FIXED_TO_INT(near_min));

    // Test that fractional part is preserved in calculations
    FIXED_Q1616 a = FIXED_FROM_FLOAT(1.5f);
    FIXED_Q1616 b = FIXED_FROM_FLOAT(2.5f);
    FIXED_Q1616 sum = FIXED_add(a, b);
    TEST_ASSERT_EQUAL(FIXED_FROM_INT(4), sum);

    FIXED_Q1616 diff = FIXED_sub(b, a);
    TEST_ASSERT_EQUAL(FIXED_ONE, diff);
}

/**
 * Test fixed-point conversions near epsilon
 */
void test_FIXED_conversions_near_epsilon(void)
{
    // Test values around the epsilon threshold
    double q1616_epsilon = FIXED_TO_FLOAT(FIXED_EPSILON);
    TEST_ASSERT_TRUE(q1616_epsilon == 1.0f / 65536.0f);
    TEST_ASSERT_TRUE(FIXED_FROM_FLOAT(q1616_epsilon) == FIXED_EPSILON);
}

/**
 * Test boundary conditions
 */
void test_FIXED_boundaries(void)
{
    // Test operations at the boundaries
    FIXED_Q1616 max_safe_int = FIXED_FROM_INT(32767);
    FIXED_Q1616 min_safe_int = FIXED_FROM_INT(-32768);

    // These should work without overflow
    TEST_ASSERT_EQUAL(FIXED_FROM_INT(32766), FIXED_sub(max_safe_int, FIXED_ONE));
    TEST_ASSERT_EQUAL(FIXED_FROM_INT(-32767), FIXED_add(min_safe_int, FIXED_ONE));

    // Test with maximum and minimum fixed values
    TEST_ASSERT_EQUAL(INT32_MAX, FIXED_add(INT32_MAX, 1));
    TEST_ASSERT_EQUAL(INT32_MIN, FIXED_sub(INT32_MIN, 1));

    // Test float conversion boundaries
    float max_float = FIXED_TO_FLOAT(INT32_MAX);
    float min_float = FIXED_TO_FLOAT(INT32_MIN);

    TEST_ASSERT_TRUE(max_float > 30000.0f); // Much larger than 32767
    TEST_ASSERT_TRUE(min_float < -30000.0f); // Much smaller than -32768
}

/**
 * Test mathematical properties
 */
void test_FIXED_math_properties(void)
{
    FIXED_Q1616 a = FIXED_FROM_FLOAT(3.14f);
    FIXED_Q1616 b = FIXED_FROM_FLOAT(2.71f);
    FIXED_Q1616 c = FIXED_FROM_FLOAT(1.41f);

    // Commutative property of addition
    TEST_ASSERT_EQUAL(FIXED_add(a, b), FIXED_add(b, a));

    // Commutative property of multiplication
    TEST_ASSERT_EQUAL(FIXED_mul(a, b), FIXED_mul(b, a));

    // Associative property of addition
    FIXED_Q1616 add1 = FIXED_add(FIXED_add(a, b), c);
    FIXED_Q1616 add2 = FIXED_add(a, FIXED_add(b, c));
    TEST_ASSERT_TRUE(add1 - add2 == 0);

    // Distributive property: a * (b + c) = a * b + a * c
    FIXED_Q1616 left = FIXED_mul(a, FIXED_add(b, c));
    FIXED_Q1616 right = FIXED_add(FIXED_mul(a, b), FIXED_mul(a, c));
    TEST_ASSERT_TRUE(left - right == 0);

    // Identity elements
    TEST_ASSERT_EQUAL(a, FIXED_add(a, FIXED_ZERO));
    TEST_ASSERT_EQUAL(a, FIXED_mul(a, FIXED_ONE));

    // Inverse operations
    FIXED_Q1616 div_result = FIXED_div(a, b);
    FIXED_Q1616 mul_back = FIXED_mul(div_result, b);
    TEST_ASSERT_TRUE(mul_back - a == 0);
}

/**
 * Test FIXED_to_string function
 */
void test_FIXED_to_string(void)
{
    char buffer[32];
    char* result;

    // Test basic values
    result = FIXED_to_string(FIXED_ZERO, buffer, sizeof(buffer));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("0.0", result);

    result = FIXED_to_string(FIXED_ONE, buffer, sizeof(buffer));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("1.0", result);

    result = FIXED_to_string(FIXED_FROM_INT(-1), buffer, sizeof(buffer));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("-1.0", result);

    // Test fractional values
    result = FIXED_to_string(FIXED_HALF, buffer, sizeof(buffer));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("0.5", result);

    result = FIXED_to_string(FIXED_FROM_FLOAT(3.14159f), buffer, sizeof(buffer));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("3.14159", result);

    result = FIXED_to_string(FIXED_FROM_FLOAT(-2.71828f), buffer, sizeof(buffer));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("-2.71828", result);

    // Test boundary values
    result = FIXED_to_string(FIXED_MAX, buffer, sizeof(buffer));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("32767.99998", result);

    result = FIXED_to_string(FIXED_MIN, buffer, sizeof(buffer));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("-32768.0", result);

    // Test epsilon (smallest fractional increment)
    result = FIXED_to_string(FIXED_EPSILON, buffer, sizeof(buffer));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("0.00002", result); // 1/65536 ≈ 0.00001526

    // Test error conditions
    TEST_ASSERT_NULL(FIXED_to_string(FIXED_ONE, NULL, 16));
    TEST_ASSERT_NULL(FIXED_to_string(FIXED_ONE, buffer, 12)); // Buffer too small

    // Test minimum buffer size with worst case
    char min_buffer[13];
    result = FIXED_to_string(FIXED_FROM_INT(-32768), min_buffer, sizeof(min_buffer));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("-32768.0", result);
}
