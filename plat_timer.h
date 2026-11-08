#pragma once

#include "plat_time.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct plat_timer_t plat_timer_t;

typedef enum : uint8_t {
    PLAT_TIMER_TYPE_ONESHOT,
    PLAT_TIMER_TYPE_PERIODIC,
} plat_timer_type_t;

typedef void (*plat_timer_handler_t)(plat_timer_t* plat_timer, void* ctx);

plat_timer_t* plat_timer_start(
    plat_timer_type_t type,
    plat_time_ms_t duration,
    plat_timer_handler_t handler,
    void* ctx);
void plat_timer_stop(plat_timer_t* plat_timer);

#ifdef __cplusplus
}
#endif
