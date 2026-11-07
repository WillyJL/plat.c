#include "plat_marker.h"

#include "plat_log.h"

#include <stdio.h>

#define PFX "Marker"

void plat_marker_detail(const char* file, int line, const char* func, const char* fmt, ...) {
    char text[128];
    va_list args;
    va_start(args, fmt);
    vsnprintf(text, sizeof(text), fmt, args);
    va_end(args);

    PLAT_LOG_E(PFX, "%s:%d in %s(): %s (%s)", file, line, func, text, PLAT_APP_VERSION);
}
