#pragma once

#include <stdint.h>

typedef uint8_t plat_os_t;

#define _PLAT_OS_MIN    (1)
#define PLAT_OS_WINDOWS (1)
#define PLAT_OS_LINUX   (2)
#define PLAT_OS_MACOS   (3)
#define _PLAT_OS_MAX    (4)

#if defined(_WIN32)
#define PLAT_OS PLAT_OS_WINDOWS
#elif defined(linux) || defined(__linux) || defined(__linux__)
#define PLAT_OS PLAT_OS_LINUX
#elif defined(__APPLE__)
#define PLAT_OS PLAT_OS_MACOS
#else
#error "OS not supported!"
#endif
