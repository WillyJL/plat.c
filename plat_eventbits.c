#define _GNU_SOURCE
#include "plat_eventbits.h"

#include "plat_assert.h"
#include "plat_os.h"
#include "plat_time.h"

#include <errno.h> // IWYU pragma: keep
#include <stdlib.h>

#if PLAT_OS == PLAT_OS_WINDOWS

#include <windows.h>

struct plat_eventbits_t {
    SRWLOCK lock;
    CONDITION_VARIABLE cond;
    plat_eventbits_bits_t bits;
};

plat_eventbits_t* plat_eventbits_alloc() {
    plat_eventbits_t* plat_eventbits = malloc(sizeof(plat_eventbits_t));
    plat_eventbits->bits = 0;
    InitializeSRWLock(&plat_eventbits->lock);
    InitializeConditionVariable(&plat_eventbits->cond);
    return plat_eventbits;
}

void plat_eventbits_free(plat_eventbits_t* plat_eventbits) {
    plat_assert(plat_eventbits);

    // SRW locks and condition variables need no cleanup
    free(plat_eventbits);
}

void plat_eventbits_set(plat_eventbits_t* plat_eventbits, plat_eventbits_bits_t bits) {
    plat_assert(plat_eventbits);
    plat_assert(bits);

    AcquireSRWLockExclusive(&plat_eventbits->lock);
    plat_eventbits->bits |= bits;
    ReleaseSRWLockExclusive(&plat_eventbits->lock);
    WakeConditionVariable(&plat_eventbits->cond);
}

void plat_eventbits_clear(plat_eventbits_t* plat_eventbits, plat_eventbits_bits_t bits) {
    plat_assert(plat_eventbits);
    plat_assert(bits);

    AcquireSRWLockExclusive(&plat_eventbits->lock);
    plat_eventbits->bits &= ~bits;
    ReleaseSRWLockExclusive(&plat_eventbits->lock);
}

plat_eventbits_bits_t plat_eventbits_wait(
    plat_eventbits_t* plat_eventbits,
    plat_eventbits_bits_t bits,
    bool all,
    bool clear,
    plat_timeout_t timeout) {
    plat_assert(plat_eventbits);
    plat_assert(bits);

    plat_time_ms_t exp = 0;
    if(timeout != PLAT_TIMEOUT_FOREVER) {
        exp = plat_time_get_mono_ms() + timeout;
    }
    plat_eventbits_bits_t recv = 0;
    AcquireSRWLockExclusive(&plat_eventbits->lock);
    while(true) {
        recv |= (plat_eventbits->bits & bits);
        bool done = all ? (recv == bits) : (recv != 0);
        if(done) {
            if(clear) plat_eventbits->bits &= ~recv;
            break;
        }
        DWORD wait_ms = INFINITE;
        if(timeout != PLAT_TIMEOUT_FOREVER) {
            plat_time_ms_t now = plat_time_get_mono_ms();
            if(now >= exp) break;
            wait_ms = (DWORD)MIN((uint64_t)(exp - now), (uint64_t)(INFINITE - 1));
        }
        if(!SleepConditionVariableSRW(&plat_eventbits->cond, &plat_eventbits->lock, wait_ms, 0)) {
            plat_assert(GetLastError() == ERROR_TIMEOUT);
        }
    }
    ReleaseSRWLockExclusive(&plat_eventbits->lock);
    return recv;
}

#elif PLAT_OS == PLAT_OS_LINUX || PLAT_OS == PLAT_OS_MACOS

#include "_plat_os.h"

#include <pthread.h>

struct plat_eventbits_t {
    pthread_mutex_t p_mutex;
    pthread_cond_t p_cond;
    plat_eventbits_bits_t bits;
};

plat_eventbits_t* plat_eventbits_alloc() {
    plat_eventbits_t* plat_eventbits = malloc(sizeof(plat_eventbits_t));
    plat_eventbits->bits = 0;
    plat_assert(pthread_mutex_init(&plat_eventbits->p_mutex, NULL) == 0);
    plat_assert(pthread_cond_init(&plat_eventbits->p_cond, NULL) == 0);
    return plat_eventbits;
}

void plat_eventbits_free(plat_eventbits_t* plat_eventbits) {
    plat_assert(plat_eventbits);

    plat_assert(pthread_mutex_destroy(&plat_eventbits->p_mutex) == 0);
    plat_assert(pthread_cond_destroy(&plat_eventbits->p_cond) == 0);
    free(plat_eventbits);
}

void plat_eventbits_set(plat_eventbits_t* plat_eventbits, plat_eventbits_bits_t bits) {
    plat_assert(plat_eventbits);
    plat_assert(bits);

    plat_assert(pthread_mutex_lock(&plat_eventbits->p_mutex) == 0);
    plat_eventbits->bits |= bits;
    plat_assert(pthread_mutex_unlock(&plat_eventbits->p_mutex) == 0);
    plat_assert(pthread_cond_signal(&plat_eventbits->p_cond) == 0);
}

void plat_eventbits_clear(plat_eventbits_t* plat_eventbits, plat_eventbits_bits_t bits) {
    plat_assert(plat_eventbits);
    plat_assert(bits);

    plat_assert(pthread_mutex_lock(&plat_eventbits->p_mutex) == 0);
    plat_eventbits->bits &= ~bits;
    plat_assert(pthread_mutex_unlock(&plat_eventbits->p_mutex) == 0);
}

plat_eventbits_bits_t plat_eventbits_wait(
    plat_eventbits_t* plat_eventbits,
    plat_eventbits_bits_t bits,
    bool all,
    bool clear,
    plat_timeout_t timeout) {
    plat_assert(plat_eventbits);
    plat_assert(bits);

    if(timeout == PLAT_TIMEOUT_FOREVER) {
        plat_eventbits_bits_t recv = 0;
        bool first = true;
        while(true) {
            plat_assert(pthread_mutex_lock(&plat_eventbits->p_mutex) == 0);
            if(!first) {
                plat_assert(
                    pthread_cond_wait(&plat_eventbits->p_cond, &plat_eventbits->p_mutex) == 0);
            }
            first = false;
            recv |= (plat_eventbits->bits & bits);
            bool done = all ? (recv == bits) : (recv != 0);
            if(done && clear) plat_eventbits->bits &= ~recv;
            plat_assert(pthread_mutex_unlock(&plat_eventbits->p_mutex) == 0);
            if(done) return recv;
        }
    } else {
        plat_time_spec_t ts = plat_time_get_mono();
        if(timeout != PLAT_TIMEOUT_NONE) {
            ts = plat_time_spec_add_ms(ts, timeout);
        }
        plat_eventbits_bits_t recv = 0;
        bool first = true;
        while(true) {
            int lock = plat_os_pthread_mutex_clocklock(&plat_eventbits->p_mutex, &ts);
            plat_assert(lock == 0 || lock == ETIMEDOUT);
            if(lock == ETIMEDOUT) return recv;
            int wait = 0;
            if(!first) {
                wait = plat_os_pthread_cond_clockwait(
                    &plat_eventbits->p_cond,
                    &plat_eventbits->p_mutex,
                    &ts);
                plat_assert(wait == 0 || wait == ETIMEDOUT);
            }
            first = false;
            recv |= (plat_eventbits->bits & bits);
            bool done = all ? (recv == bits) : (recv != 0);
            if(done && clear) plat_eventbits->bits &= ~recv;
            plat_assert(pthread_mutex_unlock(&plat_eventbits->p_mutex) == 0);
            if(done || wait == ETIMEDOUT) return recv;
        }
    }
}

#endif
