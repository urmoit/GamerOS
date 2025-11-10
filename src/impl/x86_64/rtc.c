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

    // Convert BCD to binary (proper implementation)
    *second = ((*second & 0xF0) >> 4) * 10 + (*second & 0x0F);
    *minute = ((*minute & 0xF0) >> 4) * 10 + (*minute & 0x0F);
    *hour = (((*hour & 0x70) >> 4) * 10 + (*hour & 0x0F)) | (*hour & 0x80); // Preserve 12/24 hour bit
}