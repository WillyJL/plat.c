#include "plat_time.h"

#include "plat_assert.h"
#include "plat_crash.h"

#include <unistd.h>

#define PFX "Time"

plat_static_assert(sizeof(time_t) == 8, "Time is not 64 bits");

void plat_time_sleep_ms(plat_time_ms_t ms) {
    plat_time_sleep_us(ms * 1000);
}

void plat_time_sleep_us(plat_time_us_t us) {
    usleep(us);
}

plat_time_spec_t plat_time_get_real() {
    plat_time_spec_t ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts;
}

plat_time_spec_t plat_time_get_mono() {
    plat_time_spec_t ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts;
}

plat_time_ms_t plat_time_get_mono_ms() {
    plat_time_spec_t ts = plat_time_get_mono();
    plat_time_ms_t ms = ts.tv_sec * 1000 + ts.tv_nsec / 1'000'000;
    return ms;
}

plat_time_repr_t plat_time_get_repr(plat_time_spec_t ts) {
    struct tm tm;
    localtime_r(&ts.tv_sec, &tm);
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
    m_string_printf(str,
                    "%04d-%02d-%02dT%02d:%02d:%02d.%06d",
                    tr.year,
                    tr.month,
                    tr.day,
                    tr.hour,
                    tr.minute,
                    tr.second,
                    ts.tv_nsec / 1000);
}
