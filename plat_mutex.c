#define _GNU_SOURCE
#include "plat_mutex.h"

#include "_plat_os_i.h"
#include "plat_assert.h"
#include "plat_crash.h"
#include "plat_os.h"

#include <errno.h> // IWYU pragma: keep
#include <stdlib.h>

#if PLAT_OS == PLAT_OS_WINDOWS

#include <windows.h>

// Windows mutex objects are always recursive and support timed waits natively
struct plat_mutex_t {
    HANDLE handle;
};

plat_mutex_t* plat_mutex_alloc(plat_mutex_type_t type) {
    switch(type) {
    case PLAT_MUTEX_TYPE_NORMAL:
    case PLAT_MUTEX_TYPE_RECURSIVE:
        break;
    default:
        plat_crash("Wrong plat_mutex_type_t");
    }
    plat_mutex_t* plat_mutex = malloc(sizeof(plat_mutex_t));
    plat_mutex->handle = CreateMutexW(NULL, FALSE, NULL);
    plat_assert(plat_mutex->handle != NULL);
    return plat_mutex;
}

void plat_mutex_free(plat_mutex_t* plat_mutex) {
    plat_assert(plat_mutex);

    plat_assert(CloseHandle(plat_mutex->handle));
    free(plat_mutex);
}

bool plat_mutex_acquire(plat_mutex_t* plat_mutex, plat_timeout_t timeout) {
    plat_assert(plat_mutex);

    return plat_os_windows_wait(plat_mutex->handle, timeout);
}

void plat_mutex_release(plat_mutex_t* plat_mutex) {
    plat_assert(plat_mutex);

    plat_assert(ReleaseMutex(plat_mutex->handle));
}

#elif PLAT_OS == PLAT_OS_LINUX

#include "plat_time.h"

#include <pthread.h>

struct plat_mutex_t {
    pthread_mutex_t p_mutex;
};

plat_mutex_t* plat_mutex_alloc(plat_mutex_type_t type) {
    plat_mutex_t* plat_mutex = malloc(sizeof(plat_mutex_t));
    pthread_mutexattr_t p_mutex_attr;
    plat_assert(pthread_mutexattr_init(&p_mutex_attr) == 0);
    int p_mutex_type;
    switch(type) {
    case PLAT_MUTEX_TYPE_NORMAL:
        p_mutex_type = PTHREAD_MUTEX_NORMAL;
        break;
    case PLAT_MUTEX_TYPE_RECURSIVE:
        p_mutex_type = PTHREAD_MUTEX_RECURSIVE;
        break;
    default:
        plat_crash("Wrong plat_mutex_type_t");
    }
    plat_assert(pthread_mutexattr_settype(&p_mutex_attr, p_mutex_type) == 0);
    plat_assert(pthread_mutex_init(&plat_mutex->p_mutex, &p_mutex_attr) == 0);
    plat_assert(pthread_mutexattr_destroy(&p_mutex_attr) == 0);
    return plat_mutex;
}

void plat_mutex_free(plat_mutex_t* plat_mutex) {
    plat_assert(plat_mutex);

    plat_assert(pthread_mutex_destroy(&plat_mutex->p_mutex) == 0);
    free(plat_mutex);
}

bool plat_mutex_acquire(plat_mutex_t* plat_mutex, plat_timeout_t timeout) {
    plat_assert(plat_mutex);

    if(timeout == PLAT_TIMEOUT_FOREVER) {
        plat_assert(pthread_mutex_lock(&plat_mutex->p_mutex) == 0);
        return true;
    } else if(timeout == PLAT_TIMEOUT_NONE) {
        int lock = pthread_mutex_trylock(&plat_mutex->p_mutex);
        plat_assert(lock == 0 || lock == EBUSY);
        return lock == 0;
    } else {
        plat_time_spec_t ts = plat_time_get_mono();
        ts = plat_time_spec_add_ms(ts, timeout);
        int lock = plat_os_pthread_mutex_clocklock(&plat_mutex->p_mutex, &ts);
        plat_assert(lock == 0 || lock == ETIMEDOUT);
        return lock == 0;
    }
}

void plat_mutex_release(plat_mutex_t* plat_mutex) {
    plat_assert(plat_mutex);

    plat_assert(pthread_mutex_unlock(&plat_mutex->p_mutex) == 0);
}

#elif PLAT_OS == PLAT_OS_MACOS

#include "plat_time.h"

#include <pthread.h>

// macOS has no timed mutex lock, so implement the mutex with a condition variable
struct plat_mutex_t {
    pthread_mutex_t p_mutex;
    pthread_cond_t p_cond;
    plat_mutex_type_t type;
    bool locked;
    pthread_t owner;
    size_t count;
};

plat_mutex_t* plat_mutex_alloc(plat_mutex_type_t type) {
    switch(type) {
    case PLAT_MUTEX_TYPE_NORMAL:
    case PLAT_MUTEX_TYPE_RECURSIVE:
        break;
    default:
        plat_crash("Wrong plat_mutex_type_t");
    }
    plat_mutex_t* plat_mutex = malloc(sizeof(plat_mutex_t));
    plat_mutex->type = type;
    plat_mutex->locked = false;
    plat_mutex->count = 0;
    plat_assert(pthread_mutex_init(&plat_mutex->p_mutex, NULL) == 0);
    plat_assert(pthread_cond_init(&plat_mutex->p_cond, NULL) == 0);
    return plat_mutex;
}

void plat_mutex_free(plat_mutex_t* plat_mutex) {
    plat_assert(plat_mutex);
    plat_assert(!plat_mutex->locked);

    plat_assert(pthread_mutex_destroy(&plat_mutex->p_mutex) == 0);
    plat_assert(pthread_cond_destroy(&plat_mutex->p_cond) == 0);
    free(plat_mutex);
}

bool plat_mutex_acquire(plat_mutex_t* plat_mutex, plat_timeout_t timeout) {
    plat_assert(plat_mutex);

    pthread_t self = pthread_self();
    plat_time_spec_t ts;
    if(timeout != PLAT_TIMEOUT_FOREVER && timeout != PLAT_TIMEOUT_NONE) {
        ts = plat_time_get_mono();
        ts = plat_time_spec_add_ms(ts, timeout);
    }

    plat_assert(pthread_mutex_lock(&plat_mutex->p_mutex) == 0);
    if(plat_mutex->type == PLAT_MUTEX_TYPE_RECURSIVE && plat_mutex->locked && pthread_equal(plat_mutex->owner, self)) {
        plat_mutex->count++;
        plat_assert(pthread_mutex_unlock(&plat_mutex->p_mutex) == 0);
        return true;
    }
    while(plat_mutex->locked) {
        if(timeout == PLAT_TIMEOUT_NONE) break;
        if(timeout == PLAT_TIMEOUT_FOREVER) {
            plat_assert(pthread_cond_wait(&plat_mutex->p_cond, &plat_mutex->p_mutex) == 0);
        } else {
            int wait = plat_os_pthread_cond_clockwait(&plat_mutex->p_cond, &plat_mutex->p_mutex, &ts);
            plat_assert(wait == 0 || wait == ETIMEDOUT);
            if(wait == ETIMEDOUT) break;
        }
    }
    bool acquired = !plat_mutex->locked;
    if(acquired) {
        plat_mutex->locked = true;
        plat_mutex->owner = self;
        plat_mutex->count = 1;
    }
    plat_assert(pthread_mutex_unlock(&plat_mutex->p_mutex) == 0);
    return acquired;
}

void plat_mutex_release(plat_mutex_t* plat_mutex) {
    plat_assert(plat_mutex);

    plat_assert(pthread_mutex_lock(&plat_mutex->p_mutex) == 0);
    plat_assert(plat_mutex->locked && pthread_equal(plat_mutex->owner, pthread_self()));
    bool unlocked = --plat_mutex->count == 0;
    if(unlocked) plat_mutex->locked = false;
    plat_assert(pthread_mutex_unlock(&plat_mutex->p_mutex) == 0);
    if(unlocked) plat_assert(pthread_cond_signal(&plat_mutex->p_cond) == 0);
}

#endif
