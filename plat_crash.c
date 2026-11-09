#include "plat_crash.h"

#include "plat_log.h"
#include "plat_os.h"

#include <stdarg.h>
#include <stdio.h>

#if PLAT_OS == PLAT_OS_WINDOWS
#include <stdlib.h>
#include <windows.h>
#elif PLAT_OS == PLAT_OS_LINUX || PLAT_OS == PLAT_OS_MACOS
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#endif

#define PFX "Crash"

PLAT_NO_RETURN static void plat_crash_trap_or_die() {
#if PLAT_OS == PLAT_OS_WINDOWS
    // Fail-fast bypasses any SEH/vectored handlers and goes straight to the OS
    RaiseFailFastException(NULL, NULL, FAIL_FAST_GENERATE_EXCEPTION_ADDRESS);
#elif PLAT_OS == PLAT_OS_LINUX || PLAT_OS == PLAT_OS_MACOS
    // Make sure nothing installed by us or a library swallows it
    struct sigaction sa = {0};
    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGTRAP, &sa, NULL);
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGTRAP);
    pthread_sigmask(SIG_UNBLOCK, &set, NULL);

    // Break into debugger or crash and dump core
    raise(SIGTRAP);
#endif

    // Reached if the debugger resumed without delivering the signal
    // (e.g. gdb "continue", which doesn't pass SIGTRAP by default)
    _exit(1);
}

PLAT_NO_RETURN void plat_crash_detail(
    const char* file,
    int line,
    const char* func,
    const char* fmt,
    ...) {
    char text[128];
    va_list args;
    va_start(args, fmt);
    vsnprintf(text, sizeof(text), fmt, args);
    va_end(args);

    PLAT_LOG_E(PFX, "%s:%d in %s(): %s (%s)", file, line, func, text, PLAT_APP_VERSION);

    plat_crash_trap_or_die();
}
