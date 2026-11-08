#pragma once

#include "plat_defs.h"

#include <stdarg.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum : uint8_t {
    PLAT_LOG_LEVEL_NONE,
    PLAT_LOG_LEVEL_ERROR,
    PLAT_LOG_LEVEL_WARN,
    PLAT_LOG_LEVEL_INFO,
    PLAT_LOG_LEVEL_DEBUG,
    PLAT_LOG_LEVEL_TRACE,
    _PLAT_LOG_LEVEL_MAX,
} plat_log_level_t;

void plat_log_init();

void plat_log_vprintf(plat_log_level_t level, const char* pfx, const char* fmt, va_list args);

#define PLAT_LOGV_E(pfx, fmt, args) plat_log_vprintf(PLAT_LOG_LEVEL_ERROR, pfx, fmt, args)
#define PLAT_LOGV_W(pfx, fmt, args) plat_log_vprintf(PLAT_LOG_LEVEL_WARN, pfx, fmt, args)
#define PLAT_LOGV_I(pfx, fmt, args) plat_log_vprintf(PLAT_LOG_LEVEL_INFO, pfx, fmt, args)
#define PLAT_LOGV_D(pfx, fmt, args) plat_log_vprintf(PLAT_LOG_LEVEL_DEBUG, pfx, fmt, args)
#define PLAT_LOGV_T(pfx, fmt, args) plat_log_vprintf(PLAT_LOG_LEVEL_TRACE, pfx, fmt, args)

void plat_log_printf(plat_log_level_t level, const char* pfx, const char* fmt, ...) PLAT_PRINTF(3);

#define PLAT_LOG_E(pfx, fmt, ...) plat_log_printf(PLAT_LOG_LEVEL_ERROR, pfx, fmt, ##__VA_ARGS__)
#define PLAT_LOG_W(pfx, fmt, ...) plat_log_printf(PLAT_LOG_LEVEL_WARN, pfx, fmt, ##__VA_ARGS__)
#define PLAT_LOG_I(pfx, fmt, ...) plat_log_printf(PLAT_LOG_LEVEL_INFO, pfx, fmt, ##__VA_ARGS__)
#define PLAT_LOG_D(pfx, fmt, ...) plat_log_printf(PLAT_LOG_LEVEL_DEBUG, pfx, fmt, ##__VA_ARGS__)
#define PLAT_LOG_T(pfx, fmt, ...) plat_log_printf(PLAT_LOG_LEVEL_TRACE, pfx, fmt, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif
