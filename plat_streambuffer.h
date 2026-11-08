#pragma once

#include "plat_types.h"

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct plat_streambuffer_t plat_streambuffer_t;

plat_streambuffer_t* plat_streambuffer_alloc(size_t size);
void plat_streambuffer_free(plat_streambuffer_t* plat_streambuffer);

size_t plat_streambuffer_send(plat_streambuffer_t* plat_streambuffer, const void* data, size_t size,
                              plat_timeout_t timeout);
size_t plat_streambuffer_recv(plat_streambuffer_t* plat_streambuffer, void* data, size_t size, size_t want,
                              plat_timeout_t timeout);

size_t plat_streambuffer_size(plat_streambuffer_t* plat_streambuffer);
size_t plat_streambuffer_avail_send(plat_streambuffer_t* plat_streambuffer);
size_t plat_streambuffer_avail_recv(plat_streambuffer_t* plat_streambuffer);
void plat_streambuffer_drain(plat_streambuffer_t* plat_streambuffer);

#ifdef __cplusplus
}
#endif
