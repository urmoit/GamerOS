#include "../../intf/mouse.h"
#include "../../intf/ports.h"
#include "../../intf/pic.h"

#define MOUSE_PORT   0x60
#define MOUSE_STATUS 0x64
#define MOUSE_ABIT   0x20
#define MOUSE_BBIT   0x10
#define MOUSE_CBIT   0x08
#define MOUSE_DBIT   0x04
#define MOUSE_VBIT   0x02
#define MOUSE_PS2    0xA8
#define MOUSE_WRITE  0xD4
#define MOUSE_ENABLE 0xF4
#define MOUSE_DEFAULT 0xF6

static uint8_t mouse_cycle = 0;
static int8_t mouse_byte[3];
static int32_t mouse_x = 0;
static int32_t mouse_y = 0;

void mouse_wait(uint8_t type) {
    uint32_t timeout = 100000;
    if (type == 0) {
        while (timeout--) {
            if ((inb(MOUSE_STATUS) & MOUSE_BBIT) == 0) {
                return;
            }
        }
        return;
    } else {
        while (timeout--) {
            if (inb(MOUSE_STATUS) & MOUSE_ABIT) {
                return;
            }
        }
        return;
    }
}

void mouse_write(uint8_t write) {
    mouse_wait(1);
    outb(MOUSE_STATUS, MOUSE_WRITE);
    mouse_wait(1);
    outb(MOUSE_PORT, write);
}

uint8_t mouse_read() {
    mouse_wait(0);
    return inb(MOUSE_PORT);
}

void mouse_handler() {
    uint8_t status = inb(MOUSE_STATUS);
    if (!(status & MOUSE_ABIT)) {
        pic_eoi(12); // End of interrupt for IRQ12
        return;
    }

    switch (mouse_cycle) {
        case 0:
            mouse_byte[0] = mouse_read();
            mouse_cycle++;
            break;
        case 1:
            mouse_byte[1] = mouse_read();
            mouse_cycle++;
            break;
        case 2:
            mouse_byte[2] = mouse_read();
            // Clamp mouse coordinates to prevent overflow
            mouse_x += (int8_t)mouse_byte[1];
            mouse_y -= (int8_t)mouse_byte[2];
            if (mouse_x < 0) mouse_x = 0;
            if (mouse_y < 0) mouse_y = 0;
            // Note: No upper bounds checking as screen size may vary

            // Handle mouse buttons
            if (mouse_byte[0] & 0x01) { // Left button
                // Handle left click
            }
            if (mouse_byte[0] & 0x02) { // Right button
                // Handle right click
            }
            if (mouse_byte[0] & 0x04) { // Middle button
                // Handle middle click
            }

            mouse_cycle = 0;
            break;
    }
    pic_eoi(12); // End of interrupt for IRQ12
}

void mouse_init() {
    uint8_t status_byte;

    // Enable the auxiliary mouse device
    mouse_wait(1);
    outb(MOUSE_STATUS, MOUSE_PS2);
    mouse_read(); // Acknowledge

    // Enable the mouse interrupt
    mouse_wait(1);
    outb(MOUSE_STATUS, MOUSE_ENABLE);
    mouse_read(); // Acknowledge

    // Set default settings
    mouse_write(MOUSE_DEFAULT);
    mouse_read(); // Acknowledge

    // Enable packet streaming
    mouse_write(MOUSE_ENABLE);
    mouse_read(); // Acknowledge

    // Set mouse handler in IDT (not implemented here)
    // For now, we assume IDT is set up to handle IRQ12
}