#pragma once

#include "plat_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

PLAT_NO_RETURN void plat_crash_detail(
    const char* file,
    int line,
    const char* func,
    const char* fmt,
    ...) PLAT_PRINTF(4);

#define plat_crash(fmt, ...) plat_crash_detail(__FILE__, __LINE__, __FUNC__, fmt, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif
