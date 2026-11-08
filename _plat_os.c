#define _GNU_SOURCE
#include "_plat_os.h"

#include <stdio.h>
#include <string.h>

const char* plat_os_strerror(int err, char* buf, size_t size) {
#if PLAT_OS == PLAT_OS_WINDOWS
    if(strerror_s(buf, size, err) != 0) {
        snprintf(buf, size, "Unknown error %d", err);
    }
    return buf;
#elif PLAT_OS == PLAT_OS_LINUX
    // GNU strerror_r() due to _GNU_SOURCE, may return a static string instead of filling buf
    return strerror_r(err, buf, size);
#elif PLAT_OS == PLAT_OS_MACOS
    // XSI strerror_r(), still fills buf with "Unknown error: N" on EINVAL
    strerror_r(err, buf, size);
    return buf;
#endif
}

#if PLAT_OS == PLAT_OS_WINDOWS

#include "plat_assert.h"
#include "plat_defs.h"

#include <errno.h>
#include <stdlib.h>

wchar_t* plat_os_windows_utf8_to_wide(const char* str) {
    // Length includes the null terminator since cbMultiByte is -1
    int len = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str, -1, NULL, 0);
    if(len <= 0) {
        errno = EILSEQ;
        return NULL;
    }
    wchar_t* wide = malloc((size_t)len * sizeof(wchar_t));
    if(wide == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    int res = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str, -1, wide, len);
    plat_assert(res == len);
    return wide;
}

bool plat_os_windows_wait(HANDLE handle, plat_timeout_t timeout) {
    if(timeout == PLAT_TIMEOUT_FOREVER) {
        DWORD wait = WaitForSingleObject(handle, INFINITE);
        plat_assert(wait == WAIT_OBJECT_0 || wait == WAIT_ABANDONED);
        return true;
    }
    uint64_t remaining = timeout;
    while(true) {
        DWORD chunk = (DWORD)MIN(remaining, (uint64_t)(INFINITE - 1));
        DWORD wait = WaitForSingleObject(handle, chunk);
        if(wait == WAIT_OBJECT_0 || wait == WAIT_ABANDONED) return true;
        plat_assert(wait == WAIT_TIMEOUT);
        remaining -= chunk;
        if(remaining == 0) return false;
    }
}

#elif PLAT_OS == PLAT_OS_LINUX || PLAT_OS == PLAT_OS_MACOS

#if PLAT_OS == PLAT_OS_MACOS
#include <errno.h> // IWYU pragma: keep
#endif

int plat_os_pthread_mutex_clocklock(pthread_mutex_t* p_mutex, const plat_time_spec_t* deadline) {
#if PLAT_OS == PLAT_OS_LINUX
    return pthread_mutex_clocklock(p_mutex, CLOCK_MONOTONIC, deadline);
#elif PLAT_OS == PLAT_OS_MACOS
    // macOS has no timed mutex lock, internal mutexes are only held briefly so a plain lock is fine
    UNUSED(deadline);
    return pthread_mutex_lock(p_mutex);
#endif
}

int plat_os_pthread_cond_clockwait(
    pthread_cond_t* p_cond,
    pthread_mutex_t* p_mutex,
    const plat_time_spec_t* deadline) {
#if PLAT_OS == PLAT_OS_LINUX
    return pthread_cond_clockwait(p_cond, p_mutex, CLOCK_MONOTONIC, deadline);
#elif PLAT_OS == PLAT_OS_MACOS
    // macOS has no clock selection for conditions, but supports relative timeouts
    plat_time_spec_t now = plat_time_get_mono();
    if(now.tv_sec > deadline->tv_sec ||
       (now.tv_sec == deadline->tv_sec && now.tv_nsec >= deadline->tv_nsec)) {
        return ETIMEDOUT;
    }
    struct timespec rel = {
        .tv_sec = deadline->tv_sec - now.tv_sec,
        .tv_nsec = deadline->tv_nsec - now.tv_nsec,
    };
    if(rel.tv_nsec < 0) {
        rel.tv_nsec += 1'000'000'000;
        rel.tv_sec -= 1;
    }
    return pthread_cond_timedwait_relative_np(p_cond, p_mutex, &rel);
#endif
}

#endif
