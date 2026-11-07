#define _GNU_SOURCE
#include "plat_mutex.h"

#include "plat_assert.h"
#include "plat_crash.h"
#include "plat_time.h"

#include <pthread.h>
#include <stdlib.h>
#include <sys/errno.h>

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
        int lock = pthread_mutex_clocklock(&plat_mutex->p_mutex, CLOCK_MONOTONIC, &ts);
        plat_assert(lock == 0 || lock == ETIMEDOUT);
        return lock == 0;
    }
}

void plat_mutex_release(plat_mutex_t* plat_mutex) {
    plat_assert(plat_mutex);

    plat_assert(pthread_mutex_unlock(&plat_mutex->p_mutex) == 0);
}
