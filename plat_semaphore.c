#define _GNU_SOURCE
#include "plat_semaphore.h"

#include "plat_assert.h"
#include "plat_time.h"

#include <pthread.h>
#include <stdlib.h>
#include <sys/errno.h>

struct plat_semaphore_t {
    pthread_mutex_t p_mutex;
    pthread_cond_t p_cond;
    size_t max_count;
    size_t count;
};

plat_semaphore_t* plat_semaphore_alloc(size_t max_count, size_t initial_count) {
    plat_assert(max_count);
    plat_assert(initial_count <= max_count);

    plat_semaphore_t* plat_semaphore = malloc(sizeof(plat_semaphore_t));
    plat_semaphore->max_count = max_count;
    plat_semaphore->count = initial_count;
    plat_assert(pthread_mutex_init(&plat_semaphore->p_mutex, NULL) == 0);
    plat_assert(pthread_cond_init(&plat_semaphore->p_cond, NULL) == 0);
    return plat_semaphore;
}

void plat_semaphore_free(plat_semaphore_t* plat_semaphore) {
    plat_assert(plat_semaphore);

    plat_assert(pthread_mutex_destroy(&plat_semaphore->p_mutex) == 0);
    plat_assert(pthread_cond_destroy(&plat_semaphore->p_cond) == 0);
    free(plat_semaphore);
}

bool plat_semaphore_take(plat_semaphore_t* plat_semaphore, plat_timeout_t timeout) {
    plat_assert(plat_semaphore);

    if(timeout == PLAT_TIMEOUT_FOREVER) {
        plat_assert(pthread_mutex_lock(&plat_semaphore->p_mutex) == 0);
        while(plat_semaphore->count == 0) {
            plat_assert(pthread_cond_wait(&plat_semaphore->p_cond, &plat_semaphore->p_mutex) == 0);
        }
        plat_semaphore->count--;
        plat_assert(pthread_mutex_unlock(&plat_semaphore->p_mutex) == 0);
        return true;
    } else if(timeout == PLAT_TIMEOUT_NONE) {
        plat_assert(pthread_mutex_lock(&plat_semaphore->p_mutex) == 0);
        bool taken = plat_semaphore->count > 0;
        if(taken) plat_semaphore->count--;
        plat_assert(pthread_mutex_unlock(&plat_semaphore->p_mutex) == 0);
        return taken;
    } else {
        plat_time_spec_t ts = plat_time_get_mono();
        ts = plat_time_spec_add_ms(ts, timeout);
        int lock = pthread_mutex_clocklock(&plat_semaphore->p_mutex, CLOCK_MONOTONIC, &ts);
        plat_assert(lock == 0 || lock == ETIMEDOUT);
        if(lock == ETIMEDOUT) return false;
        while(plat_semaphore->count == 0) {
            int wait = pthread_cond_clockwait(&plat_semaphore->p_cond, &plat_semaphore->p_mutex, CLOCK_MONOTONIC, &ts);
            plat_assert(wait == 0 || wait == ETIMEDOUT);
            if(wait == ETIMEDOUT) break;
        }
        bool taken = plat_semaphore->count > 0;
        if(taken) plat_semaphore->count--;
        plat_assert(pthread_mutex_unlock(&plat_semaphore->p_mutex) == 0);
        return taken;
    }
}

void plat_semaphore_give(plat_semaphore_t* plat_semaphore) {
    plat_assert(plat_semaphore);

    plat_assert(pthread_mutex_lock(&plat_semaphore->p_mutex) == 0);
    plat_assert(plat_semaphore->count < plat_semaphore->max_count);
    plat_semaphore->count++;
    plat_assert(pthread_mutex_unlock(&plat_semaphore->p_mutex) == 0);
    plat_assert(pthread_cond_signal(&plat_semaphore->p_cond) == 0);
}
