#pragma once

#include "plat_crash.h" // IWYU pragma: keep

#define plat_static_assert _Static_assert

#define plat_assert(expr)                                                                                              \
    do {                                                                                                               \
        if(!(expr)) {                                                                                                  \
            plat_crash("plat_assert(%s)", #expr);                                                                      \
        }                                                                                                              \
    } while(0)
