#pragma once

#include <stdint.h>

typedef enum : uint64_t {
    PLAT_TIMEOUT_NONE = 0,
    PLAT_TIMEOUT_FOREVER = 0xFFFFFFFFFFFFFFFF,
} plat_timeout_t;
