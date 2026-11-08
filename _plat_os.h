#pragma once

// Private helpers shared by OS-specific implementations, not part of the public API

#include "plat_os.h"

#include <stdbool.h>
#include <stddef.h>

// Thread-safe strerror(), the message is written to buf or may point to a static string.
// Lives here because GNU and XSI strerror_r() cannot both be used from one translation unit.
const char* plat_os_strerror(int err, char* buf, size_t size);

// Recommended buffer size for plat_os_strerror()
#define PLAT_OS_STRERROR_BUF_LEN (128)

#if PLAT_OS == PLAT_OS_WINDOWS

#include "plat_types.h"

#include <windows.h>

// Wait for a waitable handle, supports timeouts longer than what a DWORD can hold.
// Returns true if the object was signaled, false on timeout.
bool plat_os_windows_wait(HANDLE handle, plat_timeout_t timeout);

// Convert a UTF-8 string to a newly allocated UTF-16 string, free() it when done.
// Returns NULL and sets errno (EILSEQ on invalid UTF-8, ENOMEM) on failure.
wchar_t* plat_os_windows_utf8_to_wide(const char* str);

#elif PLAT_OS == PLAT_OS_LINUX || PLAT_OS == PLAT_OS_MACOS

#include "plat_time.h"

#include <pthread.h>

// Lock a pthread mutex with an absolute CLOCK_MONOTONIC deadline.
int plat_os_pthread_mutex_clocklock(pthread_mutex_t* p_mutex, const plat_time_spec_t* deadline);

// Wait on a pthread condition with an absolute CLOCK_MONOTONIC deadline.
int plat_os_pthread_cond_clockwait(pthread_cond_t* p_cond, pthread_mutex_t* p_mutex, const plat_time_spec_t* deadline);

#endif
