#include "plat_time.h"

#include "plat_assert.h"
#include "plat_crash.h"
#include "plat_os.h"

#if PLAT_OS == PLAT_OS_WINDOWS
#include <windows.h>
#else
#include <unistd.h>
#endif

#define PFX "Time"

plat_static_assert(sizeof(time_t) == 8, "Time is not 64 bits");

void plat_time_sleep_ms(plat_time_ms_t ms) {
    plat_time_sleep_us(ms * 1000);
}

void plat_time_sleep_us(plat_time_us_t us) {
#if PLAT_OS == PLAT_OS_WINDOWS
    // Sleep() only has millisecond (or worse) resolution, use a high resolution waitable timer if available
    HANDLE timer = CreateWaitableTimerExW(
        NULL,
        NULL,
        CREATE_WAITABLE_TIMER_HIGH_RESOLUTION,
        TIMER_ALL_ACCESS);
    if(!timer) timer = CreateWaitableTimerExW(NULL, NULL, 0, TIMER_ALL_ACCESS);
    plat_assert(timer != NULL);
    // Negative due time is relative, in 100ns units
    LARGE_INTEGER due = {.QuadPart = -(LONGLONG)us * 10};
    plat_assert(SetWaitableTimer(timer, &due, 0, NULL, NULL, FALSE));
    plat_assert(WaitForSingleObject(timer, INFINITE) == WAIT_OBJECT_0);
    plat_assert(CloseHandle(timer));
#else
    usleep(us);
#endif
}

plat_time_spec_t plat_time_get_real() {
    plat_time_spec_t ts;
#if PLAT_OS == PLAT_OS_WINDOWS
    // FILETIME is in 100ns units since 1601-01-01
    FILETIME ft;
    GetSystemTimePreciseAsFileTime(&ft);
    uint64_t time = ((uint64_t)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
    time -= 116'444'736'000'000'000ULL; // 1601-01-01 to 1970-01-01
    ts.tv_sec = (time_t)(time / 10'000'000);
    ts.tv_nsec = (long)(time % 10'000'000) * 100;
#else
    clock_gettime(CLOCK_REALTIME, &ts);
#endif
    return ts;
}

plat_time_spec_t plat_time_get_mono() {
    plat_time_spec_t ts;
#if PLAT_OS == PLAT_OS_WINDOWS
    static LARGE_INTEGER freq = {};
    if(freq.QuadPart == 0) plat_assert(QueryPerformanceFrequency(&freq));
    LARGE_INTEGER counter;
    plat_assert(QueryPerformanceCounter(&counter));
    ts.tv_sec = (time_t)(counter.QuadPart / freq.QuadPart);
    ts.tv_nsec = (long)((counter.QuadPart % freq.QuadPart) * 1'000'000'000 / freq.QuadPart);
#else
    clock_gettime(CLOCK_MONOTONIC, &ts);
#endif
    return ts;
}

plat_time_ms_t plat_time_get_mono_ms() {
    plat_time_spec_t ts = plat_time_get_mono();
    plat_time_ms_t ms = ts.tv_sec * 1000 + ts.tv_nsec / 1'000'000;
    return ms;
}

plat_time_repr_t plat_time_get_repr(plat_time_spec_t ts) {
    struct tm tm;
#if PLAT_OS == PLAT_OS_WINDOWS
    plat_assert(localtime_s(&tm, &ts.tv_sec) == 0);
#else
    localtime_r(&ts.tv_sec, &tm);
#endif
    plat_time_repr_t tr = {
        .year = 1900 + tm.tm_year,
        .month = 1 + tm.tm_mon,
        .day = tm.tm_mday,
        .week_day = (tm.tm_wday == 0) ? 7 : tm.tm_wday,
        .hour = tm.tm_hour,
        .minute = tm.tm_min,
        .second = tm.tm_sec,
    };
    return tr;
}

plat_time_spec_t plat_time_spec_add_ms(plat_time_spec_t ts, plat_time_ms_t add) {
    ts.tv_sec += add / 1000;
    ts.tv_nsec += (add % 1000) * 1'000'000;
    if(ts.tv_nsec >= 1'000'000'000) {
        ts.tv_nsec -= 1'000'000'000;
        ts.tv_sec += 1;
    }
    return ts;
}

bool plat_time_is_leap_year(uint16_t year) {
    return (((year) % 4 == 0) && ((year) % 100 != 0)) || ((year) % 400 == 0);
}

uint8_t plat_time_days_in_month(uint8_t month /* 1..12 */, uint16_t year /* Actual year */) {
    switch(month) {
    case 1:
    case 3:
    case 5:
    case 7:
    case 8:
    case 10:
    case 12:
        return 31;
    case 4:
    case 6:
    case 9:
    case 11:
        return 30;
    case 2:
        if(plat_time_is_leap_year(year)) {
            return 29;
        } else {
            return 28;
        }
    default:
        plat_crash("Invalid month");
    }
}

void plat_time_format_iso(plat_time_spec_t ts, m_string_ptr str) {
    plat_time_repr_t tr = plat_time_get_repr(ts);
    m_string_printf(
        str,
        "%04d-%02d-%02dT%02d:%02d:%02d.%06d",
        tr.year,
        tr.month,
        tr.day,
        tr.hour,
        tr.minute,
        tr.second,
        ts.tv_nsec / 1000);
}
