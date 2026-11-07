#include "plat_log.h"

#include "plat_assert.h"
#include "plat_mlib.h"
#include "plat_mutex.h"
#include "plat_time.h"

#include <inttypes.h>
#include <stddef.h>

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

void plat_log_init() {
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

    m_string_init_printf(str,
                         PLAT_LOG_COLOR_FMT(PLAT_LOG_COLOR_GREY) "%10" PRIu64 " " PLAT_LOG_COLOR_FMT(
                             "%s") "[%c][%s] " PLAT_LOG_COLOR_FMT_RESET,
                         plat_time_get_mono_ms(),
#ifdef PLAT_LOG_COLOR
                         color,
#endif
                         letter,
                         pfx);

    m_string_cat_vprintf(str, fmt, args);

    m_string_cat(str, PLAT_LOG_COLOR_FMT_RESET "\r\n");

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
