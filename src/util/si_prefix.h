/**
 * @file si_prefix.h
 * @brief SI prefix constants for unit conversions
 *
 * This file provides constants for SI prefixes from tera (10^12) to pico
 * (10^-12). These constants can be used for unit conversions and scaling
 * operations.
 */

#ifndef SI_PREFIX_H
#define SI_PREFIX_H

#ifdef __cplusplus
extern "C" {
#endif

/* Alternative integer-based constants for operations requiring larger ranges */

/** Tera as integer constant */
#define SI_TERA_INT 1000000000000ULL

/** Giga as integer constant */
#define SI_GIGA_INT 1000000000UL

/** Mega as integer constant */
#define SI_MEGA_INT 1000000UL

/** Kilo as integer constant */
#define SI_KILO_INT 1000UL

/** Milli as divisor (for multiplication) */
#define SI_MILLI_DIV 1000UL

/** Micro as divisor (for multiplication) */
#define SI_MICRO_DIV 1000000UL

/** Nano as divisor (for multiplication) */
#define SI_NANO_DIV 1000000000UL

/** Pico as divisor (for multiplication) */
#define SI_PICO_DIV 1000000000000ULL

#ifdef __cplusplus
}
#endif

#endif /* SI_PREFIX_H */
