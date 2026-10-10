#include <timezone.h>
#include <rtc.h>
#include <fs.h>
#include <string.h>

// Standard IANA/Windows time zones with their standard offsets and, where they
// observe one, the daylight-saving rule that governs them.
//
// The RTC is treated as UTC. Local time is UTC plus the zone offset, with
// daylight saving applied when the instant falls inside that zone's transition
// window for the year.

static const tz_info_t g_zones[] = {
    { "UTC-12:00  International Date Line West", -720, TZ_DST_NONE },
    { "UTC-11:00  Midway Island, Samoa",          -660, TZ_DST_NONE },
    { "UTC-10:00  Hawaii",                        -600, TZ_DST_NONE },
    { "UTC-10:00  Midway Islands",                -600, TZ_DST_NONE },
    { "UTC-09:30  Marquesas Islands",             -570, TZ_DST_NONE },
    { "UTC-09:00  Alaska",                        -540, TZ_DST_US   },
    { "UTC-09:00  Coordinated Universal Time-09",  -540, TZ_DST_NONE },
    { "UTC-08:00  Pacific Time (US & Canada)",    -480, TZ_DST_US   },
    { "UTC-08:00  Tijuana",                       -480, TZ_DST_US   },
    { "UTC-07:00  Mountain Time (US & Canada)",   -420, TZ_DST_US   },
    { "UTC-07:00  Arizona",                       -420, TZ_DST_NONE },
    { "UTC-07:00  Chihuahua, La Paz",             -420, TZ_DST_NONE },
    { "UTC-07:00  Mazatlan",                      -420, TZ_DST_NONE },
    { "UTC-06:00  Central Time (US & Canada)",    -360, TZ_DST_US   },
    { "UTC-06:00  Guadalajara, Mexico City",      -360, TZ_DST_NONE },
    { "UTC-06:00  Costa Rica, Tegucigalpa",       -360, TZ_DST_NONE },
    { "UTC-05:00  Eastern Time (US & Canada)",    -300, TZ_DST_US   },
    { "UTC-05:00  Bogota, Lima, Quito",           -300, TZ_DST_NONE },
    { "UTC-05:00  Panama",                        -300, TZ_DST_NONE },
    { "UTC-04:00  Atlantic Time (Canada)",        -240, TZ_DST_US   },
    { "UTC-04:00  Caracas, La Paz",               -240, TZ_DST_NONE },
    { "UTC-04:00  Santiago",                      -240, TZ_DST_NONE },
    { "UTC-04:00  Puerto Rico",                   -240, TZ_DST_NONE },
    { "UTC-03:30  Newfoundland",                  -210, TZ_DST_US   },
    { "UTC-03:00  Buenos Aires",                  -180, TZ_DST_NONE },
    { "UTC-03:00  Greenland",                     -180, TZ_DST_NONE },
    { "UTC-03:00  Montevideo, Sao Paulo",         -180, TZ_DST_NONE },
    { "UTC-02:00  Azores",                         -60, TZ_DST_EU   },
    { "UTC-01:00  Cape Verde Islands",             -60, TZ_DST_NONE },
    { "UTC+00:00  Casablanca",                       0, TZ_DST_NONE },
    { "UTC+00:00  Dublin, Edinburgh, Lisbon, London", 0, TZ_DST_EU  },
    { "UTC+00:00  Monrovia, Reykjavik",              0, TZ_DST_NONE },
    { "UTC+01:00  Belgrade, Bratislava, Budapest",  60, TZ_DST_EU   },
    { "UTC+01:00  Central European Standard Time",  60, TZ_DST_EU   },
    { "UTC+01:00  Copenhagen, Madrid, Paris",       60, TZ_DST_EU   },
    { "UTC+01:00  West Central Africa",             60, TZ_DST_NONE },
    { "UTC+02:00  Cairo",                          120, TZ_DST_NONE },
    { "UTC+02:00  Eastern European Standard Time", 120, TZ_DST_EU   },
    { "UTC+02:00  Helsinki, Kyiv, Riga, Sofia",    120, TZ_DST_EU   },
    { "UTC+02:00  Johannesburg, Harare",          120, TZ_DST_NONE },
    { "UTC+03:00  East Africa, Nairobi",           180, TZ_DST_NONE },
    { "UTC+03:00  Istanbul",                       180, TZ_DST_NONE },
    { "UTC+03:00  Moscow, St. Petersburg",         180, TZ_DST_NONE },
    { "UTC+03:30  Iran",                           210, TZ_DST_NONE },
    { "UTC+04:00  Abu Dhabi, Baku, Muscat, Tbilisi", 240, TZ_DST_NONE },
    { "UTC+04:30  Kabul",                          270, TZ_DST_NONE },
    { "UTC+05:00  Karachi",                        300, TZ_DST_NONE },
    { "UTC+05:00  Islamabad",                      300, TZ_DST_NONE },
    { "UTC+05:30  Chennai, Kolkata, Mumbai, New Delhi", 330, TZ_DST_NONE },
    { "UTC+05:30  Sri Jayawardenepura",            330, TZ_DST_NONE },
    { "UTC+05:45  Kathmandu",                      345, TZ_DST_NONE },
    { "UTC+06:00  Astana, Dhaka",                  360, TZ_DST_NONE },
    { "UTC+06:30  Yangon",                         390, TZ_DST_NONE },
    { "UTC+07:00  Bangkok, Hanoi, Jakarta",        420, TZ_DST_NONE },
    { "UTC+08:00  Beijing, Chongqing, Hong Kong, Singapore", 480, TZ_DST_NONE },
    { "UTC+08:00  Kuala Lumpur, Perth, Taipei",    480, TZ_DST_NONE },
    { "UTC+08:00  Manila",                         480, TZ_DST_NONE },
    { "UTC+08:45  Eucla",                          525, TZ_DST_NONE },
    { "UTC+09:00  Osaka, Sapporo, Tokyo",          540, TZ_DST_NONE },
    { "UTC+09:00  Darwin",                         540, TZ_DST_NONE },
    { "UTC+09:30  Adelaide",                       570, TZ_DST_AU   },
    { "UTC+09:30  Broken Hill",                    570, TZ_DST_AU   },
    { "UTC+10:00  Brisbane",                       600, TZ_DST_NONE },
    { "UTC+10:00  Canberra, Melbourne, Sydney",    600, TZ_DST_AU   },
    { "UTC+10:00  Guam, Port Moresby",             600, TZ_DST_NONE },
    { "UTC+10:30  Lord Howe Island",               630, TZ_DST_NONE },
    { "UTC+11:00  Solomon Is., New Caledonia",    660, TZ_DST_NONE },
    { "UTC+11:00  Vladivostok",                    660, TZ_DST_NONE },
    { "UTC+12:00  Anadyr, Petropavlovsk-Kamchatsky", 720, TZ_DST_NONE },
    { "UTC+12:00  Auckland, Wellington",           720, TZ_DST_NZ   },
    { "UTC+12:00  Fiji",                           720, TZ_DST_NONE },
    { "UTC+13:00  Nuku'alofa, Tongatapu",          780, TZ_DST_NONE },
    { "UTC+13:00  Samoa (formerly)",               780, TZ_DST_NONE },
    { "UTC+14:00  Kiritimati",                     840, TZ_DST_NONE },
};

#define TZ_COUNT ((int)(sizeof(g_zones) / sizeof(g_zones[0])))

static int g_current = -1;   // -1 forces a lazy load
static uint8_t g_loaded = 0;

// ---- Calendar helpers (closed form; no libgcc, no runtime division) ---------

static int32_t tz_days_from_civil(int32_t y, int32_t m, int32_t d) {
    y -= (m <= 2) ? 1 : 0;
    const int32_t era = (y >= 0 ? y : y - 399) / 400;
    const int32_t yoe = y - era * 400;
    const int32_t doy = (153 * (m + ((m > 2) ? -3 : 9)) + 2) / 5 + d - 1;
    const int32_t doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + doe - 719468;
}

static void tz_civil_from_days(int32_t z, int32_t* y, int32_t* m, int32_t* d) {
    z += 719468;
    const int32_t era = (z >= 0 ? z : z - 146096) / 146097;
    const int32_t doe = z - era * 146097;
    const int32_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    const int32_t yr = yoe + era * 400;
    const int32_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const int32_t mp = (5 * doy + 2) / 153;
    const int32_t dd = doy - (153 * mp + 2) / 5 + 1;
    const int32_t mm = mp + ((mp < 10) ? 3 : -9);
    *y = yr + ((mm <= 2) ? 1 : 0);
    *m = mm;
    *d = dd;
}

// 1970-01-01 was a Thursday, so shifting by 4 makes this a 0=Sunday index.
static int tz_weekday(int32_t y, int32_t m, int32_t d) {
    int32_t days = tz_days_from_civil(y, m, d);
    int32_t w = (days + 4) % 7;
    if (w < 0) w += 7;
    return (int)w;
}

// Day-of-month of the nth weekday in a month. nth is 1-based; 5 means "last".
static int32_t tz_nth_weekday(int32_t y, int32_t m, int weekday, int nth) {
    int32_t first_wd = tz_weekday(y, m, 1);
    int32_t delta = weekday - first_wd;
    if (delta < 0) delta += 7;
    int32_t day = 1 + delta + (int32_t)(nth - 1) * 7;
    // Walk back if we ran past the end of the month.
    int32_t len = (m == 2) ? (((y % 4 == 0 && y % 100 != 0) || y % 400 == 0) ? 29 : 28)
                          : ((m == 4 || m == 6 || m == 9 || m == 11) ? 30 : 31);
    while (day > len) day -= 7;
    return day;
}

// UTC instant of a transition. minutes_of_day_utc may be negative for zones
// far east of Greenwich.
static uint32_t tz_boundary(int32_t y, int32_t m, int weekday, int nth,
                            int32_t minutes_of_day_utc) {
    int32_t day = tz_nth_weekday(y, m, weekday, nth);
    int32_t days = tz_days_from_civil(y, m, day);
    return (uint32_t)((int64_t)days * 86400 + minutes_of_day_utc * 60);
}

// ---- Public API -------------------------------------------------------------

int tz_count(void) {
    return TZ_COUNT;
}

const tz_info_t* tz_get(int index) {
    if (index < 0 || index >= TZ_COUNT) return 0;
    return &g_zones[index];
}

int tz_find_by_name(const char* name) {
    if (!name) return -1;
    for (int i = 0; i < TZ_COUNT; i++) {
        if (strcmp(g_zones[i].name, name) == 0) return i;
    }
    return -1;
}

static int tz_is_dst(int rule, int32_t std_off, uint32_t utc) {
    if (rule == TZ_DST_NONE) return 0;

    int32_t y, m, d;
    tz_civil_from_days((int32_t)(utc / 86400u), &y, &m, &d);

    uint32_t start, end;
    switch (rule) {
        case TZ_DST_US:
            // 02:00 local standard -> 02:00 local daylight (01:00 local std).
            start = tz_boundary(y, 3, 0, 2, 120 - std_off);
            end   = tz_boundary(y, 11, 0, 1, 60 - std_off);
            return (utc >= start && utc < end);
        case TZ_DST_EU:
            // Transitions are specified in UTC, not local time.
            start = tz_boundary(y, 3, 0, 5, 60);
            end   = tz_boundary(y, 10, 0, 5, 60);
            return (utc >= start && utc < end);
        case TZ_DST_AU:
        case TZ_DST_NZ: {
            // Southern hemisphere: the daylight window spans New Year, running
            // from October (or September for NZ) to the following April. The
            // window must therefore be anchored to the *previous* year whenever
            // the instant falls in the Jan-Apr tail; simply testing
            // "after this year's start OR before next year's end" would wrongly
            // report the whole Apr-Sep gap as daylight time.
            const int32_t start_month = (rule == TZ_DST_AU) ? 10 : 9;
            uint32_t start;
            uint32_t end;
            if (m >= start_month) {
                start = tz_boundary(y, start_month, 0, 1, 120 - std_off);
                end   = tz_boundary(y + 1, 4, 0, 1, 120 - std_off);
            } else {
                start = tz_boundary(y - 1, start_month, 0, 1, 120 - std_off);
                end   = tz_boundary(y, 4, 0, 1, 120 - std_off);
            }
            return (utc >= start && utc < end);
        }
        default:
            return 0;
    }
}

int tz_offset_minutes_at(int index, uint32_t utc_seconds) {
    const tz_info_t* z = tz_get(index);
    if (!z) return 0;
    int32_t off = z->std_offset_min;
    if (tz_is_dst(z->dst_rule, z->std_offset_min, utc_seconds)) off += 60;
    return (int)off;
}

void tz_local_time(uint32_t utc_seconds, uint16_t* out_year, uint8_t* out_month,
                   uint8_t* out_day, uint8_t* out_hour, uint8_t* out_minute,
                   uint8_t* out_second) {
    int idx = tz_current_index();
    int32_t off = tz_offset_minutes_at(idx, utc_seconds);

    int64_t local = (int64_t)utc_seconds + (int64_t)off * 60;
    if (local < 0) local = 0;

    int32_t days = (int32_t)(local / 86400);
    int32_t secs = (int32_t)(local % 86400);
    if (secs < 0) { secs += 86400; days -= 1; }

    int32_t y, m, d;
    tz_civil_from_days(days, &y, &m, &d);

    if (out_year)   *out_year   = (uint16_t)y;
    if (out_month)  *out_month  = (uint8_t)m;
    if (out_day)    *out_day    = (uint8_t)d;
    if (out_hour)   *out_hour   = (uint8_t)(secs / 3600);
    if (out_minute) *out_minute = (uint8_t)((secs % 3600) / 60);
    if (out_second) *out_second = (uint8_t)(secs % 60);
}

static void append_str(char* out, int out_size, int* pos, const char* s) {
    if (!s) return;
    while (*s && *pos + 1 < out_size) out[(*pos)++] = *s++;
}


void tz_format_offset(char* out, int out_size, int index, uint32_t utc_seconds) {
    if (!out || out_size <= 1) return;
    int pos = 0;
    out[0] = 0;
    int32_t off = tz_offset_minutes_at(index, utc_seconds);
    if (off == 0) { append_str(out, out_size, &pos, "UTC"); out[pos] = 0; return; }

    int32_t mag = (off < 0) ? -off : off;
    append_str(out, out_size, &pos, "UTC");
    out[pos++] = (off < 0) ? '-' : '+';
    if (pos < out_size) out[pos++] = '0' + (char)(mag / 60);
    if (pos + 1 < out_size) out[pos++] = '0';
    if (pos + 1 < out_size) out[pos++] = '0' + (char)(mag % 60);
    if (pos + 1 < out_size) out[pos++] = ':';
    if (pos + 1 < out_size) out[pos++] = '0';
    if (pos + 1 < out_size) out[pos++] = '0' + (char)((mag % 60) * 10 / 60);
    out[pos] = 0;
}

void tz_current_label(char* out, int out_size) {
    if (!out || out_size <= 1) return;
    out[0] = 0;
    const tz_info_t* z = tz_get(tz_current_index());
    if (!z) return;
    int pos = 0;
    // Strip the leading "UTC+xx:xx  " so the label reads as the zone name.
    const char* n = z->name;
    while (*n == 'U' || *n == 'T' || *n == 'C' || *n == '+' || *n == '-' ||
           *n == '0' || *n == '1' || *n == '2' || *n == '3' || *n == '4' ||
           *n == '5' || *n == '6' || *n == '7' || *n == '8' || *n == '9' ||
           *n == ':') n++;
    while (*n == ' ') n++;
    append_str(out, out_size, &pos, n);
    out[pos] = 0;
}

// ---- Persistence ------------------------------------------------------------

#define TZ_REG_PATH "C:/GamerOS/Registry/TIMEZONE.INI"

void tz_load(void) {
    if (g_loaded) return;
    g_loaded = 1;
    g_current = -1;

    file_t* f = fs_open_file(TZ_REG_PATH);
    if (!f) return;

    char buf[64];
    uint32_t size = fs_get_file_size(TZ_REG_PATH);
    if (size == 0 || size >= sizeof(buf)) return;
    fs_read_file(f, (uint8_t*)buf, size);
    buf[size] = 0;

    // File holds the zone index followed by a newline.
    int32_t idx = 0;
    int i = 0;
    if (buf[0] < '0' || buf[0] > '9') return;
    while (buf[i] >= '0' && buf[i] <= '9') {
        idx = idx * 10 + (buf[i] - '0');
        i++;
        if (i > 4) break;
    }
    if (idx >= 0 && idx < TZ_COUNT) g_current = idx;
}

void tz_save(void) {
    if (!g_loaded) g_loaded = 1;
    if (g_current < 0) g_current = 0;

    char buf[16];
    int pos = 0;
    int32_t v = g_current;
    char tmp[12];
    int n = 0;
    if (v == 0) tmp[n++] = '0';
    while (v > 0 && n < (int)sizeof(tmp)) { tmp[n++] = (char)('0' + (v % 10)); v /= 10; }
    while (n > 0 && pos + 1 < (int)sizeof(buf)) buf[pos++] = tmp[--n];
    buf[pos++] = '\n';

    file_t* f = fs_open_file(TZ_REG_PATH);
    if (!f) f = fs_create_file(TZ_REG_PATH);
    if (!f) return;
    fs_write_file(f, (const uint8_t*)buf, (uint32_t)pos);
}

int tz_current_index(void) {
    if (!g_loaded) tz_load();
    if (g_current < 0) {
        // Default to UTC until the user picks something.
        for (int i = 0; i < TZ_COUNT; i++) {
            if (g_zones[i].std_offset_min == 0 && g_zones[i].dst_rule == TZ_DST_EU) {
                g_current = i;
                break;
            }
        }
        if (g_current < 0) g_current = 0;
    }
    return g_current;
}

void tz_set_current_index(int index) {
    if (index < 0 || index >= TZ_COUNT) return;
    g_loaded = 1;
    g_current = index;
    tz_save();
}
