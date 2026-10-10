#ifndef RTC_H
#define RTC_H

#include "stdint.h"

void get_time(uint8_t* hour, uint8_t* minute, uint8_t* second);
void get_date(uint8_t* day, uint8_t* month, uint16_t* year, uint8_t* weekday);

// Seconds since the Unix epoch, derived from the RTC. Used to stamp file
// modification times so Explorer can show a "Date modified" column.
uint32_t get_unix_time(void);

// Inverse of the above: break a Unix timestamp back into calendar fields.
void unix_to_calendar(uint32_t unix_time, uint16_t* out_year, uint8_t* out_month,
                      uint8_t* out_day, uint8_t* out_hour, uint8_t* out_minute);

// TODO: Add unified RTC->Unix timestamp conversion helper.
// TODO: Add timezone/locale conversion helpers at interface layer.

#endif
