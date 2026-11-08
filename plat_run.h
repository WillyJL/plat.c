#pragma once

typedef void (*plat_run_func_t)(void* ctx);

#ifdef __cplusplus
extern "C" {
#endif

void plat_run_tick();

void plat_run_soon(plat_run_func_t func, void* ctx);

#ifdef __cplusplus
}
#endif
