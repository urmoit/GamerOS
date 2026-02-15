#ifndef RTC_H
#define RTC_H

#include "stdint.h"

void get_time(uint8_t* hour, uint8_t* minute, uint8_t* second);
void get_date(uint8_t* day, uint8_t* month, uint16_t* year, uint8_t* weekday);

// TODO: Add unified RTC->Unix timestamp conversion helper.
// TODO: Add timezone/locale conversion helpers at interface layer.

#endif
