#pragma once

#include "plat_types.h"

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct plat_semaphore_t plat_semaphore_t;

plat_semaphore_t* plat_semaphore_alloc(size_t max_count, size_t initial_count);
void plat_semaphore_free(plat_semaphore_t* plat_semaphore);

bool plat_semaphore_take(plat_semaphore_t* plat_semaphore, plat_timeout_t timeout);
void plat_semaphore_give(plat_semaphore_t* plat_semaphore);

#ifdef __cplusplus
}
#endif
