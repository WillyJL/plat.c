#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint64_t plat_random_make();
void plat_random_fill(void* data, size_t size);

#ifdef __cplusplus
}
#endif
