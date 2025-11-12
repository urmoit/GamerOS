#include "../../intf/rtc.h"
#include "../../intf/ports.h"

#define CMOS_ADDRESS 0x70
#define CMOS_DATA    0x71
#define RTC_SECONDS  0x00
#define RTC_MINUTES  0x02
#define RTC_HOURS    0x04
#define RTC_STATUS_B 0x0B

static uint8_t read_rtc_register(uint8_t reg) {
    outb(CMOS_ADDRESS, reg);
    return inb(CMOS_DATA);
}

void get_time(uint8_t* hour, uint8_t* minute, uint8_t* second) {
    if (!hour || !minute || !second) return; // NULL check

    uint8_t status_b = read_rtc_register(RTC_STATUS_B);
    uint8_t is_binary = status_b & 0x04; // DM bit (Data Mode: 0=BCD, 1=Binary)

    *second = read_rtc_register(RTC_SECONDS);
    *minute = read_rtc_register(RTC_MINUTES);
    *hour = read_rtc_register(RTC_HOURS);

    if (!is_binary) {
        // Convert BCD to binary
        *second = ((*second & 0xF0) >> 4) * 10 + (*second & 0x0F);
        *minute = ((*minute & 0xF0) >> 4) * 10 + (*minute & 0x0F);
        *hour = (((*hour & 0x70) >> 4) * 10 + (*hour & 0x0F)) | (*hour & 0x80); // Preserve 12/24 hour bit
    }
}