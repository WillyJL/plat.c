#pragma once

#include "plat_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

void plat_marker_detail(const char* file, int line, const char* func, const char* fmt, ...) PLAT_PRINTF(4);

#define plat_marker(fmt, ...) plat_marker_detail(__FILE__, __LINE__, __FUNC__, fmt, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif
