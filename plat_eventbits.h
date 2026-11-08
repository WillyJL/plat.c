#pragma once

#include "plat_types.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct plat_eventbits_t plat_eventbits_t;

typedef uint64_t plat_eventbits_bits_t;

#define PLAT_EVENTBITS_BITS_ALL (0xFFFFFFFFFFFFFFFF)

plat_eventbits_t* plat_eventbits_alloc();
void plat_eventbits_free(plat_eventbits_t* plat_eventbits);

void plat_eventbits_set(plat_eventbits_t* plat_eventbits, plat_eventbits_bits_t bits);
void plat_eventbits_clear(plat_eventbits_t* plat_eventbits, plat_eventbits_bits_t bits);
plat_eventbits_bits_t plat_eventbits_wait(plat_eventbits_t* plat_eventbits, plat_eventbits_bits_t bits, bool all,
                                          bool clear, plat_timeout_t timeout);

#ifdef __cplusplus
}
#endif
