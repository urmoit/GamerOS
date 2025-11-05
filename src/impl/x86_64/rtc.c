#include "../../intf/rtc.h"
#include "../../intf/ports.h"

#define CMOS_ADDRESS 0x70
#define CMOS_DATA    0x71

static uint8_t read_rtc_register(uint8_t reg) {
    outb(CMOS_ADDRESS, reg);
    return inb(CMOS_DATA);
}

void get_time(uint8_t* hour, uint8_t* minute, uint8_t* second) {
    *second = read_rtc_register(0x00);
    *minute = read_rtc_register(0x02);
    *hour = read_rtc_register(0x04);

    // Convert BCD to binary
    *second = (*second & 0x0F) + ((*second / 16) * 10);
    *minute = (*minute & 0x0F) + ((*minute / 16) * 10);
    *hour = ((*hour & 0x0F) + (((*hour & 0x70) / 16) * 10)) | (*hour & 0x80);
}