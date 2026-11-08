#pragma once

#include "plat_os.h"
#include "plat_types.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if PLAT_OS == PLAT_OS_WINDOWS
// Use the stack size from the executable header
#define PLAT_THREAD_STACK_DEFAULT 0
#elif PLAT_OS == PLAT_OS_LINUX
#include <bits/pthread_stack_min-dynamic.h>
#define PLAT_THREAD_STACK_DEFAULT PTHREAD_STACK_MIN
#elif PLAT_OS == PLAT_OS_MACOS
#include <limits.h>
#include <pthread.h>
#define PLAT_THREAD_STACK_DEFAULT PTHREAD_STACK_MIN
#endif

typedef struct plat_thread_t plat_thread_t;

typedef uint64_t plat_thread_result_t;

typedef plat_thread_result_t (*plat_thread_body_t)(void* ctx);

plat_thread_t* plat_thread_start(const char* name, plat_thread_body_t body, void* ctx, size_t stack);
bool plat_thread_join(plat_thread_t* plat_thread, plat_thread_result_t* result, plat_timeout_t timeout);

void plat_thread_yield();

#ifdef __cplusplus
}
#endif
