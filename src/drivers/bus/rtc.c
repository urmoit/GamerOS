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

// Days since 1970-01-01 for a proleptic Gregorian date. Closed-form
// (no loop, no division by a runtime value) so it is safe without libgcc.
static int32_t days_from_civil(int32_t y, int32_t m, int32_t d) {
    y -= (m <= 2) ? 1 : 0;
    const int32_t era = (y >= 0 ? y : y - 399) / 400;
    const int32_t yoe = y - era * 400;                                   // [0, 399]
    const int32_t doy = (153 * (m + ((m > 2) ? -3 : 9)) + 2) / 5 + d - 1;
    const int32_t doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;          // [0, 146096]
    return era * 146097 + doe - 719468;
}

static void civil_from_days(int32_t z, int32_t* out_y, int32_t* out_m, int32_t* out_d) {
    z += 719468;
    const int32_t era = (z >= 0 ? z : z - 146096) / 146097;
    const int32_t doe = z - era * 146097;                                // [0, 146096]
    const int32_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    const int32_t y = yoe + era * 400;
    const int32_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);         // [0, 365]
    const int32_t mp = (5 * doy + 2) / 153;                              // [0, 11]
    const int32_t d = doy - (153 * mp + 2) / 5 + 1;                     // [1, 31]
    const int32_t m = mp + ((mp < 10) ? 3 : -9);                        // [1, 12]
    *out_y = y + ((m <= 2) ? 1 : 0);
    *out_m = m;
    *out_d = d;
}

uint32_t get_unix_time(void) {
    uint8_t hour = 0, minute = 0, second = 0;
    uint8_t day = 1, month = 1, weekday = 0;
    uint16_t year = 2000;
    get_time(&hour, &minute, &second);
    get_date(&day, &month, &year, &weekday);
    int32_t days = days_from_civil((int32_t)year, (int32_t)month, (int32_t)day);
    return (uint32_t)days * 86400u + (uint32_t)hour * 3600u +
           (uint32_t)minute * 60u + (uint32_t)second;
}

void unix_to_calendar(uint32_t unix_time, uint16_t* out_year, uint8_t* out_month,
                      uint8_t* out_day, uint8_t* out_hour, uint8_t* out_minute) {
    uint32_t day_secs = unix_time % 86400u;
    int32_t days = (int32_t)(unix_time / 86400u);
    int32_t y = 1970, m = 1, d = 1;
    civil_from_days(days, &y, &m, &d);
    if (out_year)  *out_year  = (uint16_t)y;
    if (out_month) *out_month = (uint8_t)m;
    if (out_day)   *out_day   = (uint8_t)d;
    if (out_hour)  *out_hour  = (uint8_t)(day_secs / 3600u);
    if (out_minute)*out_minute= (uint8_t)((day_secs % 3600u) / 60u);
}

// TODO: Implement time setting functionality
// TODO: Add alarm and periodic interrupt support
// TODO: Implement daylight saving time handling
