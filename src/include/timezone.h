#ifndef TIMEZONE_H
#define TIMEZONE_H

#include "stdint.h"

// Daylight-saving transition rules. Offsets are in minutes east of UTC.
typedef enum {
    TZ_DST_NONE = 0,
    TZ_DST_US = 1,
    TZ_DST_EU = 2,
    TZ_DST_AU = 3,
    TZ_DST_NZ = 4
} tz_dst_rule_t;

typedef struct {
    const char* name;
    int16_t std_offset_min;
    uint8_t dst_rule;
} tz_info_t;

int tz_count(void);
const tz_info_t* tz_get(int index);
int tz_find_by_name(const char* name);

int tz_current_index(void);
void tz_set_current_index(int index);

int tz_offset_minutes_at(int index, uint32_t utc_seconds);

void tz_local_time(uint32_t utc_seconds, uint16_t* out_year, uint8_t* out_month,
                   uint8_t* out_day, uint8_t* out_hour, uint8_t* out_minute,
                   uint8_t* out_second);

void tz_current_label(char* out, int out_size);

// "UTC+05:30" / "UTC-08:00" / "UTC" for the given zone and instant.
void tz_format_offset(char* out, int out_size, int index, uint32_t utc_seconds);

// Persist / restore the selection under C:/GamerOS/Registry.
void tz_load(void);
void tz_save(void);

#endif
