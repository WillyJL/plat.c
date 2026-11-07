#pragma once

#include "plat_mlib.h"

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

typedef struct timespec plat_time_spec_t;

typedef struct {
    uint16_t year; // Actual year
    uint8_t month; // 1..12
    uint8_t day; // 1..31
    uint8_t week_day; // 1..7 Mon..Sun
    uint8_t hour; // 0..23
    uint8_t minute; // 0..59
    uint8_t second; // 0..60
} plat_time_repr_t;

typedef time_t plat_time_ms_t;
typedef time_t plat_time_us_t;

void plat_time_sleep_ms(plat_time_ms_t ms);
void plat_time_sleep_us(plat_time_us_t us);

plat_time_spec_t plat_time_get_real();
plat_time_spec_t plat_time_get_mono();
plat_time_ms_t plat_time_get_mono_ms();

plat_time_repr_t plat_time_get_repr(plat_time_spec_t ts);

plat_time_spec_t plat_time_spec_add_ms(plat_time_spec_t ts, plat_time_ms_t add);

bool plat_time_is_leap_year(uint16_t year);
uint8_t plat_time_days_in_month(uint8_t month /* 1..12 */, uint16_t year /* Actual year */);

void plat_time_format_iso(plat_time_spec_t ts, m_string_ptr str);
