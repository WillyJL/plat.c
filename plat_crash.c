#include "plat_crash.h"

#include "plat_log.h"
#include "plat_os.h"

#include <stdio.h>

#if PLAT_OS == PLAT_OS_WINDOWS
#include <stdlib.h>
#elif PLAT_OS == PLAT_OS_LINUX || PLAT_OS == PLAT_OS_MACOS
#include <unistd.h>
#endif

#define PFX "Crash"

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

    _exit(1);
}
