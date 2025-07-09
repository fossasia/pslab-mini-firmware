/**
 * @file reent-stub.c
 * @brief Test implementation of reent and errno for testing
 */

#include "reent.h"
_reent _reent_stub;
_reent * __impure_ptr = &_reent_stub;
