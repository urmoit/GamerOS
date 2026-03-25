#include "../../intf/keyboard.h"
#include "../../intf/ports.h"
#include "../../intf/pic.h"

#define KBD_DATA_PORT   0x60
#define KBD_STATUS_PORT 0x64

// Keyboard buffer
#define KBD_BUFFER_SIZE 256
static char kbd_buffer[KBD_BUFFER_SIZE];
static int kbd_read_idx = 0;
static int kbd_write_idx = 0;
static int is_extended = 0;

// US keyboard layout
static const char kbd_layout[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    '-', 0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static const char kbd_layout_shift[128] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    '-', 0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static int shift_pressed = 0;
static int caps_lock = 0;

// Add character to buffer
static void kbd_buffer_put(char c) {
    int next = (kbd_write_idx + 1) % KBD_BUFFER_SIZE;
    if (next == kbd_read_idx) {
        // Preserve newest input under burst typing by discarding oldest byte.
        kbd_read_idx = (kbd_read_idx + 1) % KBD_BUFFER_SIZE;
    }
    kbd_buffer[kbd_write_idx] = c;
    kbd_write_idx = next;
}

// Get character from buffer
char keyboard_getchar(void) {
    if (kbd_read_idx == kbd_write_idx) return 0;
    
    char c = kbd_buffer[kbd_read_idx];
    kbd_read_idx = (kbd_read_idx + 1) % KBD_BUFFER_SIZE;
    return c;
}

// Check if keyboard has input
int keyboard_has_input(void) {
    return kbd_read_idx != kbd_write_idx;
}

// Keyboard interrupt handler
void keyboard_handler(void) {
    uint8_t status = inb(KBD_STATUS_PORT);
    if (!(status & 0x01)) {
        return; // No output buffer data.
    }

    // If this byte is from the auxiliary device (mouse), consume and ignore it.
    // Without this, mouse bytes get interpreted as keyboard scancodes.
    if (status & 0x20) {
        (void)inb(KBD_DATA_PORT);
        return;
    }

    uint8_t scancode = inb(KBD_DATA_PORT);
    
    // Extended key prefix
    if (scancode == 0xE0) {
        is_extended = 1;
        return;
    }
    
    // Key release
    if (scancode & 0x80) {
        scancode &= 0x7F;
        if (scancode == 0x2A || scancode == 0x36) {
            shift_pressed = 0;
        }
        is_extended = 0;
        return;
    }
    
    // Shift press
    if (scancode == 0x2A || scancode == 0x36) {
        shift_pressed = 1;
        return;
    }
    
    // Caps lock
    if (scancode == 0x3A) {
        caps_lock = !caps_lock;
        return;
    }
    
    // Get character
    char c = 0;
    if (is_extended) {
        // Extended keys (arrows, etc)
        switch (scancode) {
            case 0x4B: c = KEY_LEFT; break;
            case 0x4D: c = KEY_RIGHT; break;
            case 0x48: c = KEY_UP; break;
            case 0x50: c = KEY_DOWN; break;
            case 0x47: c = KEY_HOME; break;
            case 0x4F: c = KEY_END; break;
        }
        is_extended = 0;
    } else {
        // Regular keys
        if (shift_pressed ^ caps_lock) {
            c = kbd_layout_shift[scancode];
        } else {
            c = kbd_layout[scancode];
        }
    }
    
    if (c) {
        kbd_buffer_put(c);
    }
}

// Keyboard polling fallback
void keyboard_poll(void) {
    int safety = 64;
    while (safety-- > 0 && (inb(KBD_STATUS_PORT) & 0x01)) {
        keyboard_handler();
    }
}

// Initialize keyboard
void keyboard_init(void) {
    // Empty buffer
    kbd_read_idx = 0;
    kbd_write_idx = 0;
    
    // Clear keyboard buffer
    while (inb(KBD_STATUS_PORT) & 0x01) {
        inb(KBD_DATA_PORT);
    }
    
    // Unmask IRQ1 (keyboard) in PIC
    uint8_t mask = inb(PIC1_DATA);
    mask &= ~(1 << 1); // Clear bit 1 to unmask IRQ1
    outb(PIC1_DATA, mask);
}

// TODO: Add configurable keyboard layouts (US/UK/DE) selectable at runtime.
// TODO: Implement key-repeat timing control for held keys.
