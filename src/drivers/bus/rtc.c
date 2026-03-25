#include <rtc.h>
#include <ports.h>

#define CMOS_ADDRESS 0x70
#define CMOS_DATA    0x71
#define RTC_SECONDS  0x00
#define RTC_MINUTES  0x02
#define RTC_HOURS    0x04
#define RTC_WEEKDAY  0x06
#define RTC_DAY      0x07
#define RTC_MONTH    0x08
#define RTC_YEAR     0x09
#define RTC_STATUS_B 0x0B

static uint8_t read_rtc_register(uint8_t reg) {
    // Preserve the NMI enable/disable bit (bit 7) when selecting CMOS register.
    // Writing directly to 0x70 without preserving bit 7 can unintentionally
    // unmask NMIs and cause spurious "Non Maskable Interrupt" exceptions.
    uint8_t prev = inb(CMOS_ADDRESS);
    outb(CMOS_ADDRESS, (prev & 0x80) | reg);
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

void get_date(uint8_t* day, uint8_t* month, uint16_t* year, uint8_t* weekday) {
    if (!day || !month || !year || !weekday) return;

    uint8_t status_b = read_rtc_register(RTC_STATUS_B);
    uint8_t is_binary = status_b & 0x04;

    uint8_t raw_day = read_rtc_register(RTC_DAY);
    uint8_t raw_month = read_rtc_register(RTC_MONTH);
    uint8_t raw_year = read_rtc_register(RTC_YEAR);
    uint8_t raw_weekday = read_rtc_register(RTC_WEEKDAY);

    if (!is_binary) {
        raw_day = (uint8_t)((((raw_day & 0xF0) >> 4) * 10) + (raw_day & 0x0F));
        raw_month = (uint8_t)((((raw_month & 0xF0) >> 4) * 10) + (raw_month & 0x0F));
        raw_year = (uint8_t)((((raw_year & 0xF0) >> 4) * 10) + (raw_year & 0x0F));
        raw_weekday = (uint8_t)((((raw_weekday & 0xF0) >> 4) * 10) + (raw_weekday & 0x0F));
    }

    *day = raw_day;
    *month = raw_month;
    *weekday = raw_weekday;
    *year = (uint16_t)(2000 + raw_year);
}

// TODO: Implement time setting functionality
// TODO: Add alarm and periodic interrupt support
// TODO: Implement daylight saving time handling
