#define _GNU_SOURCE
#include "plat_thread.h"

#if PLAT_OS != PLAT_OS_LINUX
#include "_plat_os.h"
#endif
#include "plat_assert.h"
#include "plat_os.h"
#if PLAT_OS != PLAT_OS_WINDOWS
#include "plat_time.h"
#endif

#include <errno.h> // IWYU pragma: keep
#include <stdlib.h>
#include <string.h>

#if PLAT_OS == PLAT_OS_WINDOWS
#include <windows.h>
#else
#include <pthread.h>
#include <sched.h>
#include <unistd.h>
#endif

#define PLAT_THREAD_NAME_MAX 16

struct plat_thread_t {
#if PLAT_OS == PLAT_OS_WINDOWS
    HANDLE handle;
#else
    pthread_t p_thread;
#endif
#if PLAT_OS == PLAT_OS_MACOS
    // macOS can only name the current thread, and has no timed/non-blocking join
    char name[PLAT_THREAD_NAME_MAX];
    pthread_mutex_t p_mutex;
    pthread_cond_t p_cond;
    bool done;
#endif
    plat_thread_body_t body;
    void* ctx;
    plat_thread_result_t result;
};

#if PLAT_OS == PLAT_OS_WINDOWS
static DWORD WINAPI plat_thread_body(LPVOID ctx) {
#else
static void* plat_thread_body(void* ctx) {
#endif
    plat_thread_t* plat_thread = ctx;

#if PLAT_OS == PLAT_OS_MACOS
    if(plat_thread->name[0]) pthread_setname_np(plat_thread->name);
#endif

    plat_thread->result = plat_thread->body(plat_thread->ctx);

#if PLAT_OS == PLAT_OS_MACOS
    plat_assert(pthread_mutex_lock(&plat_thread->p_mutex) == 0);
    plat_thread->done = true;
    plat_assert(pthread_mutex_unlock(&plat_thread->p_mutex) == 0);
    plat_assert(pthread_cond_broadcast(&plat_thread->p_cond) == 0);
#endif

#if PLAT_OS == PLAT_OS_WINDOWS
    return 0;
#else
    return NULL;
#endif
}

#if PLAT_OS == PLAT_OS_WINDOWS
typedef HRESULT(WINAPI* plat_thread_set_thread_description_t)(HANDLE thread, PCWSTR description);

static void plat_thread_windows_set_name(HANDLE handle, const char* name) {
    // SetThreadDescription() is only available since Windows 10 1607, so look it up at runtime
    HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll");
    if(!kernel32) return;
    plat_thread_set_thread_description_t set_thread_description =
        (plat_thread_set_thread_description_t)(void*)GetProcAddress(kernel32, "SetThreadDescription");
    if(!set_thread_description) return;
    wchar_t wname[PLAT_THREAD_NAME_MAX];
    plat_assert(MultiByteToWideChar(CP_UTF8, 0, name, -1, wname, ARRAY_LEN(wname)) > 0);
    plat_assert(SUCCEEDED(set_thread_description(handle, wname)));
}
#endif

plat_thread_t* plat_thread_start(const char* name, plat_thread_body_t body, void* ctx, size_t stack) {
    plat_assert(!name || strlen(name) < PLAT_THREAD_NAME_MAX);
    plat_assert(body);

    plat_thread_t* plat_thread = malloc(sizeof(plat_thread_t));
    plat_thread->body = body;
    plat_thread->ctx = ctx;
    plat_thread->result = 0;

#if PLAT_OS == PLAT_OS_WINDOWS
    plat_thread->handle =
        CreateThread(NULL, (SIZE_T)stack, plat_thread_body, plat_thread, STACK_SIZE_PARAM_IS_A_RESERVATION, NULL);
    plat_assert(plat_thread->handle != NULL);
    if(name) plat_thread_windows_set_name(plat_thread->handle, name);
#else
    if(stack < (size_t)PTHREAD_STACK_MIN) stack = PTHREAD_STACK_MIN;
#if PLAT_OS == PLAT_OS_MACOS
    // macOS requires the stack size to be a multiple of the page size
    size_t page_size = (size_t)sysconf(_SC_PAGESIZE);
    stack = (stack + page_size - 1) / page_size * page_size;
    if(name) {
        strcpy(plat_thread->name, name);
    } else {
        plat_thread->name[0] = '\0';
    }
    plat_thread->done = false;
    plat_assert(pthread_mutex_init(&plat_thread->p_mutex, NULL) == 0);
    plat_assert(pthread_cond_init(&plat_thread->p_cond, NULL) == 0);
#endif
    pthread_attr_t p_thread_attr;
    plat_assert(pthread_attr_init(&p_thread_attr) == 0);
    plat_assert(pthread_attr_setstacksize(&p_thread_attr, stack) == 0);
    plat_assert(pthread_create(&plat_thread->p_thread, &p_thread_attr, plat_thread_body, plat_thread) == 0);
    plat_assert(pthread_attr_destroy(&p_thread_attr) == 0);
#if PLAT_OS == PLAT_OS_LINUX
    if(name) plat_assert(pthread_setname_np(plat_thread->p_thread, name) == 0);
#endif
#endif

    return plat_thread;
}

bool plat_thread_join(plat_thread_t* plat_thread, plat_thread_result_t* result, plat_timeout_t timeout) {
    plat_assert(plat_thread);

    bool joined;
#if PLAT_OS == PLAT_OS_WINDOWS
    joined = plat_os_windows_wait(plat_thread->handle, timeout);
    if(joined) plat_assert(CloseHandle(plat_thread->handle));
#elif PLAT_OS == PLAT_OS_LINUX
    if(timeout == PLAT_TIMEOUT_FOREVER) {
        plat_assert(pthread_join(plat_thread->p_thread, NULL) == 0);
        joined = true;
    } else if(timeout == PLAT_TIMEOUT_NONE) {
        int join = pthread_tryjoin_np(plat_thread->p_thread, NULL);
        plat_assert(join == 0 || join == EBUSY);
        joined = (join == 0);
    } else {
        plat_time_spec_t ts = plat_time_get_mono();
        ts = plat_time_spec_add_ms(ts, timeout);
        int join = pthread_clockjoin_np(plat_thread->p_thread, NULL, CLOCK_MONOTONIC, &ts);
        plat_assert(join == 0 || join == ETIMEDOUT);
        joined = (join == 0);
    }
#elif PLAT_OS == PLAT_OS_MACOS
    if(timeout == PLAT_TIMEOUT_FOREVER) {
        joined = true;
    } else {
        plat_time_spec_t ts = plat_time_get_mono();
        ts = plat_time_spec_add_ms(ts, timeout);
        plat_assert(pthread_mutex_lock(&plat_thread->p_mutex) == 0);
        while(!plat_thread->done && timeout != PLAT_TIMEOUT_NONE) {
            int wait = plat_os_pthread_cond_clockwait(&plat_thread->p_cond, &plat_thread->p_mutex, &ts);
            plat_assert(wait == 0 || wait == ETIMEDOUT);
            if(wait == ETIMEDOUT) break;
        }
        joined = plat_thread->done;
        plat_assert(pthread_mutex_unlock(&plat_thread->p_mutex) == 0);
    }
    if(joined) {
        // Body has returned (or we wait forever), the thread exits imminently
        plat_assert(pthread_join(plat_thread->p_thread, NULL) == 0);
        plat_assert(pthread_mutex_destroy(&plat_thread->p_mutex) == 0);
        plat_assert(pthread_cond_destroy(&plat_thread->p_cond) == 0);
    }
#endif

    if(joined) {
        if(result) *result = plat_thread->result;
        free(plat_thread);
    }

    return joined;
}

void plat_thread_yield() {
#if PLAT_OS == PLAT_OS_WINDOWS
    SwitchToThread();
#else
    sched_yield();
#endif
}
