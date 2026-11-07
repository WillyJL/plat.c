#pragma once

typedef void (*plat_run_func_t)(void* ctx);

void plat_run_init();
void plat_run_tick();

void plat_run_soon(plat_run_func_t func, void* ctx);
