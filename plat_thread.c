#define _GNU_SOURCE
#include "plat_thread.h"

#include "plat_assert.h"
#include "plat_time.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>

struct plat_thread_t {
    pthread_t p_thread;
    plat_thread_body_t body;
    void* ctx;
};

static void* plat_thread_body(void* ctx) {
    plat_thread_t* plat_thread = ctx;
    return (void*)(uint64_t)plat_thread->body(plat_thread->ctx);
}

plat_thread_t* plat_thread_start(const char* name, plat_thread_body_t body, void* ctx, size_t stack) {
    plat_assert(!name || strlen(name) < 16);
    plat_assert(body);
    if(stack < (size_t)PTHREAD_STACK_MIN) stack = PTHREAD_STACK_MIN;

    plat_thread_t* plat_thread = malloc(sizeof(plat_thread_t));
    plat_thread->body = body;
    plat_thread->ctx = ctx;
    pthread_attr_t p_thread_attr;
    plat_assert(pthread_attr_init(&p_thread_attr) == 0);
    plat_assert(pthread_attr_setstacksize(&p_thread_attr, stack) == 0);
    plat_assert(pthread_create(&plat_thread->p_thread, &p_thread_attr, plat_thread_body, plat_thread) == 0);
    if(name) plat_assert(pthread_setname_np(plat_thread->p_thread, name) == 0);
    return plat_thread;
}

bool plat_thread_join(plat_thread_t* plat_thread, plat_thread_result_t* result, plat_timeout_t timeout) {
    plat_assert(plat_thread);

    void* p_result;
    bool joined;
    if(timeout == PLAT_TIMEOUT_FOREVER) {
        plat_assert(pthread_join(plat_thread->p_thread, &p_result) == 0);
        joined = true;
    } else if(timeout == PLAT_TIMEOUT_NONE) {
        int join = pthread_tryjoin_np(plat_thread->p_thread, &p_result);
        plat_assert(join == 0 || join == EBUSY);
        joined = (join == 0);
    } else {
        plat_time_spec_t ts = plat_time_get_mono();
        ts = plat_time_spec_add_ms(ts, timeout);
        int join = pthread_clockjoin_np(plat_thread->p_thread, &p_result, CLOCK_MONOTONIC, &ts);
        plat_assert(join == 0 || join == ETIMEDOUT);
        joined = (join == 0);
    }

    if(joined) {
        free(plat_thread);
        if(result) *result = (plat_thread_result_t)(uint64_t)p_result;
    }

    return joined;
}

void plat_thread_yield() {
    sched_yield();
}
