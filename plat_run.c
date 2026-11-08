#include "plat_run.h"

#include "plat_assert.h"
#include "plat_mlib.h"
#include "plat_mutex.h"

typedef struct {
    plat_run_func_t func;
    void* ctx;
} plat_run_func_ctx_t;

M_LIST_DUALF_DEF(plat_run_func_list, plat_run_func_ctx_t, M_POD_OPLIST)
#define M_OPL_plat_run_func_list_t() M_LIST_DUALF_OPLIST(plat_run_func_list, M_POD_OPLIST)

plat_mutex_t* plat_run_soon_mutex;
plat_run_func_list_t plat_run_soon_array;

void plat_run_init() {
    plat_run_soon_mutex = plat_mutex_alloc(PLAT_MUTEX_TYPE_RECURSIVE);
    plat_run_func_list_init(plat_run_soon_array);
}

void plat_run_tick() {
    plat_mutex_acquire(plat_run_soon_mutex, PLAT_TIMEOUT_FOREVER);

    while(!plat_run_func_list_empty_p(plat_run_soon_array)) {
        plat_run_func_ctx_t func_ctx;
        plat_run_func_list_pop_front(&func_ctx, plat_run_soon_array);
        func_ctx.func(func_ctx.ctx);
    }

    plat_mutex_release(plat_run_soon_mutex);
}

void plat_run_soon(plat_run_func_t func, void* ctx) {
    plat_assert(func);

    plat_mutex_acquire(plat_run_soon_mutex, PLAT_TIMEOUT_FOREVER);

    plat_run_func_ctx_t func_ctx = {
        .func = func,
        .ctx = ctx,
    };
    plat_run_func_list_push_back(plat_run_soon_array, func_ctx);

    plat_mutex_release(plat_run_soon_mutex);
}
