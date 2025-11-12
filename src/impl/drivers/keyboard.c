#include "../../intf/keyboard.h"
#include "../../intf/ports.h"
#include "../../intf/pic.h"

#define KBD_DATA_PORT   0x60
#define KBD_STATUS_PORT 0x64
#define KBD_IRQ         1
#define KBD_STATUS_OUTPUT_BUFFER 0x01
#define KBD_KEY_RELEASE_MASK 0x80
#define KBD_SCANCODE_MASK 0x7F

#define KEYBOARD_BUFFER_SIZE 256

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

void keyboard_handler() {
    uint8_t status;
    int8_t  scancode;

    // Read keyboard status
    status = inb(KBD_STATUS_PORT);
    // If the lowest bit is set, it means there is data in the output buffer
    if (status & KBD_STATUS_OUTPUT_BUFFER) {
        scancode = inb(KBD_DATA_PORT);

        if (scancode < 0) {
            pic_eoi(KBD_IRQ); // End of interrupt for keyboard
            return; // Key release - ignore for now
        }

        // Convert scancode to ASCII
        uint8_t scancode_index = (uint8_t)scancode & KBD_SCANCODE_MASK;
        if (scancode_index < sizeof(kbd_us)) {
            char c = kbd_us[scancode_index];
            if (c != 0) { // Only buffer printable characters
                // Add character to circular buffer
                if (buffer_count < KEYBOARD_BUFFER_SIZE) {
                    keyboard_buffer[buffer_head] = c;
                    buffer_head = (buffer_head + 1) % KEYBOARD_BUFFER_SIZE;
                    buffer_count++;
                }
                // Character is now buffered for application use
            }
        }
    }

    pic_eoi(KBD_IRQ); // End of interrupt for keyboard
}

void keyboard_init() {
    // Enable keyboard interrupts (IRQ1)
    // This involves setting up the PIC and IDT
    // For now, we assume IDT and PIC are set up to handle IRQ1

    // Initialize keyboard buffer
    buffer_head = 0;
    buffer_tail = 0;
    buffer_count = 0;
}

// Function to read character from keyboard buffer
char keyboard_read_char() {
    if (buffer_count == 0) {
        return 0; // No characters available
    }

    char c = keyboard_buffer[buffer_tail];
    buffer_tail = (buffer_tail + 1) % KEYBOARD_BUFFER_SIZE;
    buffer_count--;
    return c;
}

// Function to check if characters are available in buffer
int keyboard_has_char() {
    return buffer_count > 0;
}
