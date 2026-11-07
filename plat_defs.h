#pragma once

#include <stdint.h>
#include <sys/param.h>

#ifndef PLAT_APP_VERSION
#define PLAT_APP_VERSION "unknown"
#endif

#if BYTE_ORDER == LITTLE_ENDIAN
#define PLAT_LE16_TO_H(x) (x)
#define PLAT_LE32_TO_H(x) (x)
#define PLAT_LE64_TO_H(x) (x)
#define PLAT_H16_TO_LE(x) (x)
#define PLAT_H32_TO_LE(x) (x)
#define PLAT_H64_TO_LE(x) (x)
#define PLAT_BE16_TO_H(x) (__builtin_bswap16(x))
#define PLAT_BE32_TO_H(x) (__builtin_bswap32(x))
#define PLAT_BE64_TO_H(x) (__builtin_bswap64(x))
#define PLAT_H16_TO_BE(x) (__builtin_bswap16(x))
#define PLAT_H32_TO_BE(x) (__builtin_bswap32(x))
#define PLAT_H64_TO_BE(x) (__builtin_bswap64(x))
#elif BYTE_ORDER == BIG_ENDIAN
#define PLAT_LE16_TO_H(x) (__builtin_bswap16(x))
#define PLAT_LE32_TO_H(x) (__builtin_bswap32(x))
#define PLAT_LE64_TO_H(x) (__builtin_bswap64(x))
#define PLAT_H16_TO_LE(x) (__builtin_bswap16(x))
#define PLAT_H32_TO_LE(x) (__builtin_bswap32(x))
#define PLAT_H64_TO_LE(x) (__builtin_bswap64(x))
#define PLAT_BE16_TO_H(x) (x)
#define PLAT_BE32_TO_H(x) (x)
#define PLAT_BE64_TO_H(x) (x)
#define PLAT_H16_TO_BE(x) (x)
#define PLAT_H32_TO_BE(x) (x)
#define PLAT_H64_TO_BE(x) (x)
#else
#error huh
#endif

#define PLAT_PACKED __attribute__((packed))
#define PLAT_PRINTF(fmt) __attribute__((format(printf, fmt, fmt + 1)));
#define PLAT_NO_RETURN __attribute__((__noreturn__))

typedef __attribute__((aligned(1))) uint64_t _pad_t;

#undef MAX
#define MAX(a, b)                                                                                                      \
    ({                                                                                                                 \
        __typeof__(a) _a = (a);                                                                                        \
        __typeof__(b) _b = (b);                                                                                        \
        _a > _b ? _a : _b;                                                                                             \
    })

#undef MIN
#define MIN(a, b)                                                                                                      \
    ({                                                                                                                 \
        __typeof__(a) _a = (a);                                                                                        \
        __typeof__(b) _b = (b);                                                                                        \
        _a < _b ? _a : _b;                                                                                             \
    })

#define ABS(a) ({ (a) < 0 ? -(a) : (a); })

#define CLAMP(x, lower, upper) (MAX(lower, MIN(x, upper)))

#define MAP(x, in_min, in_max, out_min, out_max) ((x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min)

#define ARRAY_LEN(x) (sizeof(x) / sizeof(x[0]))

#undef UNUSED
#define UNUSED(X) (void)(X)

#define STR(x) #x
#define XSTR(x) STR(x)

#define SWAP_ITEMS(a, b)                                                                                               \
    ({                                                                                                                 \
        __typeof__(a) _tmp = (a);                                                                                      \
        a = b;                                                                                                         \
        b = _tmp;                                                                                                      \
    })

#define POWER_OF_2(n) (!((n) & ((n) - 1)))

#define __FUNC__ __PRETTY_FUNCTION__
