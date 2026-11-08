#pragma once

#include "plat_types.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct plat_mutex_t plat_mutex_t;

typedef enum : uint8_t {
    PLAT_MUTEX_TYPE_NORMAL,
    PLAT_MUTEX_TYPE_RECURSIVE,
} plat_mutex_type_t;

plat_mutex_t* plat_mutex_alloc(plat_mutex_type_t type);
void plat_mutex_free(plat_mutex_t* plat_mutex);

bool plat_mutex_acquire(plat_mutex_t* plat_mutex, plat_timeout_t timeout);
void plat_mutex_release(plat_mutex_t* plat_mutex);

#ifdef __cplusplus
}
#endif
