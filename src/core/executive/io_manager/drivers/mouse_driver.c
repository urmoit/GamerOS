#include "../io_manager.h"
#include <ports.h>
#include <pic.h>

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
static int32_t mouse_x = 0;
static int32_t mouse_y = 0;
static uint8_t mouse_buttons = 0;

typedef struct {
    int32_t x, y;
    uint8_t buttons;
} mouse_state_t;

static mouse_state_t current_state = {0, 0, 0};

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
            // Update mouse coordinates
            mouse_x += (int8_t)mouse_byte[1];
            mouse_y -= (int8_t)mouse_byte[2];
            if (mouse_x < 0) mouse_x = 0;
            if (mouse_y < 0) mouse_y = 0;
            // Note: No upper bounds checking as screen size may vary
            // TODO: Add screen boundary checking for mouse position based on current display resolution

            // Update button state
            mouse_buttons = mouse_byte[0] & 0x07;  // Lower 3 bits are buttons

            // Update current state
            current_state.x = mouse_x;
            current_state.y = mouse_y;
            current_state.buttons = mouse_buttons;

            mouse_cycle = 0;
            break;
    }
    pic_eoi(MOUSE_IRQ); // End of interrupt for mouse
}

int mouse_driver_init(void) {
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

    // Initialize state
    mouse_cycle = 0;
    mouse_x = mouse_y = 0;
    mouse_buttons = 0;
    current_state.x = current_state.y = 0;
    current_state.buttons = 0;

    // Set mouse handler in IDT (not implemented here)
    // For now, we assume IDT is set up to handle IRQ12

    return 0;
}

int mouse_driver_read(object_handle_t device, void* buffer, uint32_t size, uint32_t offset) {
    (void)device;
    (void)offset;

    if (size < sizeof(mouse_state_t) || !buffer) {
        return -1;
    }

    // Return current mouse state
    *(mouse_state_t*)buffer = current_state;
    return sizeof(mouse_state_t);
}

int mouse_driver_write(object_handle_t device, const void* buffer, uint32_t size, uint32_t offset) {
    (void)device;
    (void)buffer;
    (void)size;
    (void)offset;
    // Mouse is read-only
    return -1;
}

int mouse_driver_ioctl(object_handle_t device, uint32_t code, void* data) {
    (void)device;

    switch (code) {
        case 1:  // Set mouse position
            if (data) {
                mouse_state_t* state = (mouse_state_t*)data;
                mouse_x = state->x;
                mouse_y = state->y;
                current_state.x = mouse_x;
                current_state.y = mouse_y;
                return 0;
            }
            break;
        case 2:  // Get mouse position
            if (data) {
                *(mouse_state_t*)data = current_state;
                return 0;
            }
            break;
        default:
            break;
    }
    return -1;
}

int mouse_driver_shutdown(void) {
    // Disable mouse if needed
    return 0;
}

// Driver registration structure
device_driver_t mouse_driver = {
    .name = "mouse",
    .type = 2,  // Input device
    .init = mouse_driver_init,
    .read = mouse_driver_read,
    .write = mouse_driver_write,
    .ioctl = mouse_driver_ioctl,
    .shutdown = mouse_driver_shutdown
};

// TODO: Support for scroll wheel and additional mouse buttons
// TODO: Implement mouse acceleration and sensitivity settings
// TODO: Add mouse cursor visibility and shape management
// TODO: Implement double-click detection and timing