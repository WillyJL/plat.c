#pragma once

#include "plat_assert.h"

#include <limits.h>
#include <stdint.h>

typedef enum : uint64_t {
    PLAT_TIMEOUT_NONE = 0,
    PLAT_TIMEOUT_FOREVER = 0xFFFFFFFFFFFFFFFF,
} plat_timeout_t;

plat_static_assert(
    sizeof(float) * CHAR_BIT == 32,
    "float is not 32 bit on this architecture, fix the float32_t typedef.");
typedef float float32_t;

plat_static_assert(
    sizeof(double) * CHAR_BIT == 64,
    "double is not 64 bit on this architecture, fix the float64_t typedef.");
typedef double float64_t;

plat_static_assert(
    sizeof(long double) * CHAR_BIT == 128,
    "long double is not 128 bit on this architecture, fix the float128_t typedef.");
typedef long double float128_t;
