#ifndef PORTS_H
#define PORTS_H

#include "stdint.h"

// I/O port definitions
#define PIC1_COMMAND    0x20
#define PIC1_DATA       0x21
#define PIC2_COMMAND    0xA0
#define PIC2_DATA       0xA1
#define PIC_EOI         0x20

#define KBD_DATA_PORT   0x60
#define KBD_STATUS_PORT 0x64

#define CMOS_ADDRESS    0x70
#define CMOS_DATA       0x71

#define MOUSE_PORT      0x60
#define MOUSE_STATUS    0x64
#define MOUSE_ABIT      0x20
#define MOUSE_BBIT      0x10
#define MOUSE_CBIT      0x08
#define MOUSE_DBIT      0x04
#define MOUSE_VBIT      0x02
#define MOUSE_PS2       0xA8
#define MOUSE_WRITE     0xD4
#define MOUSE_ENABLE    0xF4
#define MOUSE_DEFAULT   0xF6

// VGA memory addresses
#define VGA_TEXT_BUFFER 0xB8000
#define VGA_GRAPHICS_BUFFER 0xA0000

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ( "inb %1, %0"
                   : "=a"(ret)
                   : "Nd"(port) );
    return ret;
}

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

#endif