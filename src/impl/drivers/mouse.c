#include "../../intf/mouse.h"
#include "../../intf/ports.h"
#include "../../intf/graphics.h"
#include "../../intf/pic.h"

#define MOUSE_DATA_PORT    0x60
#define MOUSE_STATUS_PORT  0x64
#define MOUSE_CMD_PORT     0x64

// Mouse state
static int32_t mouse_x = 160;
static int32_t mouse_y = 100;
static uint8_t mouse_buttons = 0;
static uint8_t mouse_cycle = 0;
static uint8_t mouse_packet[3];

// Mouse initialization
void mouse_init(void) {
    // Enable auxiliary mouse device
    outb(MOUSE_CMD_PORT, 0xA8);
    io_wait();
    
    // Enable IRQ12 in mouse controller
    outb(MOUSE_CMD_PORT, 0x20);
    io_wait();
    uint8_t status = inb(MOUSE_DATA_PORT) | 2;
    outb(MOUSE_CMD_PORT, 0x60);
    io_wait();
    outb(MOUSE_DATA_PORT, status);
    io_wait();
    
    // Send mouse defaults
    mouse_send_cmd(0xF6);
    mouse_send_cmd(0xF4);
    
    // Reset position to center
    mouse_x = current_width / 2;
    mouse_y = current_height / 2;
    
    // Unmask IRQ2 (cascade) on master PIC and IRQ12 on slave PIC
    uint8_t master_mask = inb(PIC1_DATA);
    master_mask &= ~(1 << 2); // Unmask IRQ2 (cascade to slave)
    outb(PIC1_DATA, master_mask);
    
    uint8_t slave_mask = inb(PIC2_DATA);
    slave_mask &= ~(1 << 4); // Unmask IRQ12 (mouse is IRQ12 - bit 4 on slave)
    outb(PIC2_DATA, slave_mask);
}

// Send command to mouse
void mouse_send_cmd(uint8_t cmd) {
    // Wait for mouse to accept commands
    while (inb(MOUSE_STATUS_PORT) & 0x02);
    outb(MOUSE_CMD_PORT, 0xD4);
    while (inb(MOUSE_STATUS_PORT) & 0x02);
    outb(MOUSE_DATA_PORT, cmd);
}

// Mouse interrupt handler
void mouse_handler(void) {
    uint8_t status = inb(MOUSE_STATUS_PORT);
    if (!(status & 0x01)) return; // No data
    if (!(status & 0x20)) { inb(MOUSE_DATA_PORT); return; } // Keyboard data
    
    uint8_t data = inb(MOUSE_DATA_PORT);
    
    switch (mouse_cycle) {
        case 0:
            if (data & 0x08) { // Valid packet start
                mouse_packet[0] = data;
                mouse_cycle = 1;
            }
            break;
        case 1:
            mouse_packet[1] = data;
            mouse_cycle = 2;
            break;
        case 2:
            mouse_packet[2] = data;
            // Process packet
            // Drop overflowed packets to avoid large coordinate jumps.
            if (mouse_packet[0] & 0xC0) {
                mouse_cycle = 0;
                break;
            }
            mouse_buttons = mouse_packet[0] & 0x07;
            
            // X/Y are already two's-complement signed deltas.
            int16_t dx = (int8_t)mouse_packet[1];
            
            // Y movement (signed, inverted for screen coordinates)
            int16_t dy = (int8_t)mouse_packet[2];
            
            // Update position
            mouse_x += dx;
            mouse_y -= dy; // Invert Y for screen coords
            
            // Clamp to screen bounds
            if (mouse_x < 0) mouse_x = 0;
            if (mouse_x >= (int32_t)current_width - CURSOR_SIZE) mouse_x = current_width - CURSOR_SIZE - 1;
            if (mouse_y < 0) mouse_y = 0;
            if (mouse_y >= (int32_t)current_height - CURSOR_SIZE) mouse_y = current_height - CURSOR_SIZE - 1;
            
            mouse_cycle = 0;
            break;
    }
}

// Poll mouse (for systems without interrupts)
void mouse_poll(void) {
    while (inb(MOUSE_STATUS_PORT) & 0x01) {
        mouse_handler();
    }
}

// Get mouse position
int32_t mouse_get_x(void) { return mouse_x; }
int32_t mouse_get_y(void) { return mouse_y; }
uint8_t mouse_get_buttons(void) { return mouse_buttons; }
mouse_state_t mouse_get_state(void) {
    mouse_state_t state;
    state.x = mouse_x;
    state.y = mouse_y;
    state.buttons = mouse_buttons;
    return state;
}

// Reset buttons (after processing)
void mouse_clear_buttons(void) { mouse_buttons = 0; }
