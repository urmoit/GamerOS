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
#define RTC_STATUS_A 0x0A
#define RTC_STATUS_B 0x0B

#define RTC_A_UIP    0x80  // update-in-progress
#define RTC_B_DM     0x04  // data mode: 0=BCD, 1=binary

static uint8_t read_rtc_register(uint8_t reg) {
    // Preserve the NMI enable/disable bit (bit 7) when selecting CMOS register.
    // Writing directly to 0x70 without preserving bit 7 can unintentionally
    // unmask NMIs and cause spurious "Non Maskable Interrupt" exceptions.
    uint8_t prev = inb(CMOS_ADDRESS);
    outb(CMOS_ADDRESS, (prev & 0x80) | reg);
    return inb(CMOS_DATA);
}

// Wait for the RTC to finish an in-progress update, otherwise the registers
// can be read mid-update and yield inconsistent values.
static void rtc_wait_update(void) {
    for (int guard = 0; guard < 100000; guard++) {
        if (!(read_rtc_register(RTC_STATUS_A) & RTC_A_UIP)) return;
    }
}

static uint8_t bcd_to_bin(uint8_t v) {
    return (uint8_t)(((v & 0xF0) >> 4) * 10 + (v & 0x0F));
}

void get_time(uint8_t* hour, uint8_t* minute, uint8_t* second) {
    if (!hour || !minute || !second) return; // NULL check

    uint8_t s = 0, m = 0, h = 0;
    // Retry a bounded number of times if the values shift between reads.
    for (int attempt = 0; attempt < 4; attempt++) {
        rtc_wait_update();
        uint8_t status_b = read_rtc_register(RTC_STATUS_B);
        uint8_t is_binary = status_b & RTC_B_DM;

        s = read_rtc_register(RTC_SECONDS);
        m = read_rtc_register(RTC_MINUTES);
        h = read_rtc_register(RTC_HOURS);

        if (!is_binary) {
            s = bcd_to_bin(s);
            m = bcd_to_bin(m);
            h = (uint8_t)((((h & 0x70) >> 4) * 10 + (h & 0x0F)) | (h & 0x80));
        }

        // Confirm the values settled; if not, sample again.
        if (s == read_rtc_register(RTC_SECONDS) &&
            m == read_rtc_register(RTC_MINUTES) &&
            h == read_rtc_register(RTC_HOURS)) {
            break;
        }
    }

    *second = s;
    *minute = m;
    *hour = h;
}

void get_date(uint8_t* day, uint8_t* month, uint16_t* year, uint8_t* weekday) {
    if (!day || !month || !year || !weekday) return;

    rtc_wait_update();
    uint8_t status_b = read_rtc_register(RTC_STATUS_B);
    uint8_t is_binary = status_b & RTC_B_DM;

    uint8_t raw_day = read_rtc_register(RTC_DAY);
    uint8_t raw_month = read_rtc_register(RTC_MONTH);
    uint8_t raw_year = read_rtc_register(RTC_YEAR);
    uint8_t raw_weekday = read_rtc_register(RTC_WEEKDAY);

    if (!is_binary) {
        raw_day = bcd_to_bin(raw_day);
        raw_month = bcd_to_bin(raw_month);
        raw_year = bcd_to_bin(raw_year);
        raw_weekday = bcd_to_bin(raw_weekday);
    }

    *day = raw_day;
    *month = raw_month;
    *weekday = raw_weekday;
    *year = (uint16_t)(2000 + raw_year);
}

// TODO: Implement time setting functionality
// TODO: Add alarm and periodic interrupt support
// TODO: Implement daylight saving time handling
