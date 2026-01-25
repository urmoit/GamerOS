#include "../../intf/mouse.h"
#include "../../intf/ports.h"
#include "../../intf/pic.h"

#define MOUSE_PORT     0x60
#define MOUSE_STATUS   0x64
#define MOUSE_IRQ      12
#define MOUSE_ABIT     0x20
#define MOUSE_BBIT     0x10
#define MOUSE_CBIT     0x08
#define MOUSE_DBIT     0x04
#define MOUSE_VBIT     0x02
#define MOUSE_PS2      0xA8
#define MOUSE_WRITE    0xD4
#define MOUSE_ENABLE   0xF4
#define MOUSE_DEFAULT  0xF6
#define MOUSE_TIMEOUT  100000
#define MOUSE_LEFT_BUTTON 0x01
#define MOUSE_RIGHT_BUTTON 0x02
#define MOUSE_MIDDLE_BUTTON 0x04

static uint8_t mouse_cycle = 0;
static int8_t mouse_byte[3];
static int32_t mouse_x = 160;  // Center x (320/2)
static int32_t mouse_y = 100;  // Center y (200/2)
static uint8_t mouse_buttons = 0;  // Track button state

void mouse_wait(uint8_t type) {
    uint32_t timeout = MOUSE_TIMEOUT;
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
        pic_eoi(MOUSE_IRQ); // End of interrupt for mouse
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
            // Update mouse coordinates with movement
            mouse_x += (int8_t)mouse_byte[1];
            mouse_y -= (int8_t)mouse_byte[2];
            
            // Clamp to screen bounds (VGA mode 13h: 320x200)
            if (mouse_x < 0) mouse_x = 0;
            if (mouse_x > 319) mouse_x = 319;
            if (mouse_y < 0) mouse_y = 0;
            if (mouse_y > 199) mouse_y = 199;
            
            // Store button states
            mouse_buttons = mouse_byte[0] & 0x07;

            mouse_cycle = 0;
            break;
    }
    pic_eoi(MOUSE_IRQ); // End of interrupt for mouse
}

void mouse_init() {
    // Simple initialization without complex handshaking
    // Just initialize the mouse buffer and position
    // Full mouse initialization can be complex and hardware-dependent
    
    // Initialize mouse buffer (already done at static init)
    // Mouse is ready to use with handler
    // Note: Full PS/2 mouse setup requires keyboard controller commands
    // which can hang on some systems, so we skip it for now
}

// Get complete mouse state
mouse_state_t mouse_get_state() {
    mouse_state_t state;
    state.x = mouse_x;
    state.y = mouse_y;
    state.buttons = mouse_buttons;
    return state;
}

// Get mouse X coordinate
int32_t mouse_get_x() {
    return mouse_x;
}

// Get mouse Y coordinate
int32_t mouse_get_y() {
    return mouse_y;
}

// Get button state
uint8_t mouse_get_buttons() {
    return mouse_buttons;
}

// TODO: Implement mouse cursor rendering and visibility control
// TODO: Add mouse wheel support and scroll events
// TODO: Implement mouse acceleration and sensitivity settings
// TODO: Add support for multiple mouse buttons beyond left/middle/right