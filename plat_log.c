#include "plat_log.h"

#include "_plat.h"
#include "plat_assert.h"
#include "plat_mlib.h"
#include "plat_mutex.h"
#include "plat_os.h"
#include "plat_time.h"

#include <inttypes.h>
#include <stddef.h>
#include <stdlib.h>

#if PLAT_OS == PLAT_OS_WINDOWS
#include <windows.h>
#endif

#define PFX "Log"

#define PLAT_LOG_COLOR

#ifdef PLAT_LOG_COLOR
#define PLAT_LOG_COLOR_FMT(code) "\033[0;" code "m"
#define PLAT_LOG_COLOR_FMT_RESET "\033[0m"
#else
#define PLAT_LOG_COLOR_FMT(code) ""
#define PLAT_LOG_COLOR_FMT_RESET ""
#endif

#define PLAT_LOG_COLOR_GREY "90"

#if PLAT_OS == PLAT_OS_WINDOWS
// stdout is in text mode, which already translates \n to \r\n
#define PLAT_LOG_NEWLINE "\n"
#else
#define PLAT_LOG_NEWLINE "\r\n"
#endif

static plat_mutex_t* plat_log_mutex = NULL;

static const char plat_log_level_letters[_PLAT_LOG_LEVEL_MAX] = {
    [PLAT_LOG_LEVEL_ERROR] = 'E',
    [PLAT_LOG_LEVEL_WARN] = 'W',
    [PLAT_LOG_LEVEL_INFO] = 'I',
    [PLAT_LOG_LEVEL_DEBUG] = 'D',
    [PLAT_LOG_LEVEL_TRACE] = 'T',
};
#ifdef PLAT_LOG_COLOR
static const char* plat_log_level_colors[_PLAT_LOG_LEVEL_MAX] = {
    [PLAT_LOG_LEVEL_ERROR] = "31",
    [PLAT_LOG_LEVEL_WARN] = "33",
    [PLAT_LOG_LEVEL_INFO] = "32",
    [PLAT_LOG_LEVEL_DEBUG] = "34",
    [PLAT_LOG_LEVEL_TRACE] = "35",
};
#endif

static void plat_log_tx(const void* str, size_t len) {
    fwrite(str, 1, len, stdout);
}

#if PLAT_OS == PLAT_OS_WINDOWS
static UINT plat_log_console_cp_prev = 0;

static void plat_log_console_cp_restore(void) {
    SetConsoleOutputCP(plat_log_console_cp_prev);
}
#endif

void plat_log_init() {
#if PLAT_OS == PLAT_OS_WINDOWS
    // Log strings are UTF-8, make the console interpret output as such instead of the OEM codepage.
    // The console is shared with the parent shell and keeps this setting, so restore it on exit.
    // Both fail harmlessly if there is no console, output redirected to a file or pipe is raw bytes anyway.
    plat_log_console_cp_prev = GetConsoleOutputCP();
    if(plat_log_console_cp_prev != 0 && plat_log_console_cp_prev != CP_UTF8 &&
       SetConsoleOutputCP(CP_UTF8)) {
        atexit(plat_log_console_cp_restore);
    }
#ifdef PLAT_LOG_COLOR
    // Enable ANSI escape sequences on the Windows console, fails harmlessly if not a console
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;
    if(console != INVALID_HANDLE_VALUE && GetConsoleMode(console, &mode)) {
        SetConsoleMode(console, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
#endif
#endif
    plat_log_mutex = plat_mutex_alloc(PLAT_MUTEX_TYPE_NORMAL);
}

void plat_log_vprintf(plat_log_level_t level, const char* pfx, const char* fmt, va_list args) {
    plat_assert(level < _PLAT_LOG_LEVEL_MAX);
    plat_assert(pfx);
    plat_assert(fmt);

    m_string_t str;
#ifdef PLAT_LOG_COLOR
    const char* color = plat_log_level_colors[level];
#endif
    const char letter = plat_log_level_letters[level];

    m_string_init_printf(
        str,
        PLAT_LOG_COLOR_FMT(PLAT_LOG_COLOR_GREY) "%10" PRIu64 " " // Timestamp
        PLAT_LOG_COLOR_FMT("%s") "[%c][%s] " // Level and prefix
        PLAT_LOG_COLOR_FMT_RESET,
        plat_time_get_mono_ms(),
#ifdef PLAT_LOG_COLOR
        color,
#endif
        letter,
        pfx);

    m_string_cat_vprintf(str, fmt, args);

    m_string_cat(str, PLAT_LOG_COLOR_FMT_RESET PLAT_LOG_NEWLINE);

    bool has_mutex = plat_log_mutex != NULL;
    if(has_mutex) plat_mutex_acquire(plat_log_mutex, PLAT_TIMEOUT_FOREVER);
    plat_log_tx(m_string_get_cstr(str), m_string_size(str));
    if(has_mutex) plat_mutex_release(plat_log_mutex);

    m_string_clear(str);
}

void plat_log_printf(plat_log_level_t level, const char* pfx, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    plat_log_vprintf(level, pfx, fmt, args);
    va_end(args);
}
