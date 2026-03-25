#include "../io_manager.h"
#include <ports.h>
#include <pic.h>

#define KBD_DATA_PORT   0x60
#define KBD_STATUS_PORT 0x64
#define KBD_IRQ         1
#define KBD_STATUS_OUTPUT_BUFFER 0x01
#define KBD_KEY_RELEASE_MASK 0x80
#define KBD_SCANCODE_MASK 0x7F
#define KBD_EXTENDED_PREFIX 0xE0

#define KEYBOARD_BUFFER_SIZE 256

// Special key codes (values > 127 to avoid ASCII conflict)
#define KEY_LEFT_ARROW   0x80
#define KEY_RIGHT_ARROW  0x81
#define KEY_UP_ARROW     0x82
#define KEY_DOWN_ARROW   0x83
#define KEY_HOME         0x84
#define KEY_END          0x85
#define KEY_PAGE_UP      0x86
#define KEY_PAGE_DOWN    0x87
#define KEY_INSERT       0x88
#define KEY_DELETE       0x89

static char kbd_us[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '\\',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0,
    '*',
    0,  // Alt
    ' ',    // Spacebar
    0,  // Caps lock
    0,  // F1 key
    0, 0, 0, 0, 0, 0, 0, 0,    // F2 to F10 keys
    0,  // Num lock
    0,  // Scroll Lock
    0,  // Home key
    0,  // Up arrow key
    0,  // Page up key
    '-',
    0,  // Left arrow key
    0,
    0,  // Right arrow key
    '+',
    0,  // End key
    0,  // Down arrow key
    0,  // Page down key
    0,  // Insert key
    0,  // Delete key
    0, 0, 0, 0,    // Extra keys
    0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 // All other keys are null for now
};

// Keyboard input buffer for proper input handling
static char keyboard_buffer[KEYBOARD_BUFFER_SIZE];
static volatile size_t buffer_head = 0;
static volatile size_t buffer_tail = 0;
static volatile size_t buffer_count = 0;
static volatile int is_extended = 0;  // Track if next scancode is extended

void keyboard_handler() {
    uint8_t status;
    uint8_t scancode;

    // Read keyboard status
    status = inb(KBD_STATUS_PORT);
    // If the lowest bit is set, it means there is data in the output buffer
    if (status & KBD_STATUS_OUTPUT_BUFFER) {
        scancode = inb(KBD_DATA_PORT);

        // Handle extended key prefix
        if (scancode == KBD_EXTENDED_PREFIX) {
            is_extended = 1;
            pic_eoi(KBD_IRQ);
            return;
        }

        // Ignore key releases (bit 7 set)
        if (scancode & KBD_KEY_RELEASE_MASK) {
            is_extended = 0;
            pic_eoi(KBD_IRQ);
            return;
        }

        // Process key press
        char key_to_buffer = 0;

        if (is_extended) {
            // Handle extended keys (arrow keys, etc.)
            switch (scancode) {
                case 0x4B:  // Left arrow
                    key_to_buffer = KEY_LEFT_ARROW;
                    break;
                case 0x4D:  // Right arrow
                    key_to_buffer = KEY_RIGHT_ARROW;
                    break;
                case 0x48:  // Up arrow
                    key_to_buffer = KEY_UP_ARROW;
                    break;
                case 0x50:  // Down arrow
                    key_to_buffer = KEY_DOWN_ARROW;
                    break;
                case 0x47:  // Home
                    key_to_buffer = KEY_HOME;
                    break;
                case 0x4F:  // End
                    key_to_buffer = KEY_END;
                    break;
                case 0x49:  // Page up
                    key_to_buffer = KEY_PAGE_UP;
                    break;
                case 0x51:  // Page down
                    key_to_buffer = KEY_PAGE_DOWN;
                    break;
                case 0x52:  // Insert
                    key_to_buffer = KEY_INSERT;
                    break;
                case 0x53:  // Delete
                    key_to_buffer = KEY_DELETE;
                    break;
            }
            is_extended = 0;
        } else {
            // Regular key - convert scancode to ASCII
            uint8_t scancode_index = scancode & KBD_SCANCODE_MASK;
            if (scancode_index < sizeof(kbd_us)) {
                key_to_buffer = kbd_us[scancode_index];
            }
        }

        // Buffer the key if valid
        if (key_to_buffer != 0) {
            if (buffer_count < KEYBOARD_BUFFER_SIZE) {
                keyboard_buffer[buffer_head] = key_to_buffer;
                buffer_head = (buffer_head + 1) % KEYBOARD_BUFFER_SIZE;
                buffer_count++;
            }
        }
    }

    pic_eoi(KBD_IRQ); // End of interrupt for keyboard
}

int keyboard_driver_init(void) {
    // Enable keyboard interrupts (IRQ1)
    // This involves setting up the PIC and IDT
    // For now, we assume IDT and PIC are set up to handle IRQ1

    // Initialize keyboard buffer
    buffer_head = 0;
    buffer_tail = 0;
    buffer_count = 0;

    return 0;
}

int keyboard_driver_read(object_handle_t device, void* buffer, uint32_t size, uint32_t offset) {
    (void)device;  // Not used for keyboard
    (void)offset;  // Not used for keyboard

    if (size == 0 || !buffer) {
        return -1;
    }

    uint32_t bytes_read = 0;
    char* buf = (char*)buffer;

    while (bytes_read < size && buffer_count > 0) {
        buf[bytes_read] = keyboard_buffer[buffer_tail];
        buffer_tail = (buffer_tail + 1) % KEYBOARD_BUFFER_SIZE;
        buffer_count--;
        bytes_read++;
    }

    return (int)bytes_read;
}

int keyboard_driver_write(object_handle_t device, const void* buffer, uint32_t size, uint32_t offset) {
    (void)device;
    (void)buffer;
    (void)size;
    (void)offset;
    // Keyboard is read-only
    return -1;
}

int keyboard_driver_ioctl(object_handle_t device, uint32_t code, void* data) {
    (void)device;

    switch (code) {
        case 1:  // Get available characters
            if (data) {
                *(int*)data = buffer_count;
                return 0;
            }
            break;
        case 2:  // Clear buffer
            buffer_head = 0;
            buffer_tail = 0;
            buffer_count = 0;
            return 0;
        default:
            break;
    }
    return -1;
}

int keyboard_driver_shutdown(void) {
    // Disable keyboard interrupts if needed
    return 0;
}

// Driver registration structure
device_driver_t keyboard_driver = {
    .name = "keyboard",
    .type = 1,  // Input device
    .init = keyboard_driver_init,
    .read = keyboard_driver_read,
    .write = keyboard_driver_write,
    .ioctl = keyboard_driver_ioctl,
    .shutdown = keyboard_driver_shutdown
};

// TODO: Implement keyboard LED control (caps lock, num lock, scroll lock)
// TODO: Add keyboard layout switching (QWERTY, Dvorak, international)
// TODO: Implement key repeat functionality
// TODO: Support for modifier keys (shift, ctrl, alt) tracking