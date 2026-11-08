#pragma once

#include "plat_types.h"

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct plat_messagequeue_t plat_messagequeue_t;

plat_messagequeue_t* plat_messagequeue_alloc(size_t item_count, size_t item_size);
void plat_messagequeue_free(plat_messagequeue_t* plat_messagequeue);

bool plat_messagequeue_send(
    plat_messagequeue_t* plat_messagequeue,
    const void* item,
    plat_timeout_t timeout);
bool plat_messagequeue_recv(
    plat_messagequeue_t* plat_messagequeue,
    void* item,
    plat_timeout_t timeout);

#ifdef __cplusplus
}
#endif
