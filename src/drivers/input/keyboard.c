#include <keyboard.h>
#include <ports.h>
#include <pic.h>

#define KBD_DATA_PORT   0x60
#define KBD_STATUS_PORT 0x64

#define KBD_BUFFER_SIZE 256
#define KBD_DRAIN_LIMIT 256
#define KBD_EXTENDED_MAX_AGE 4
static char kbd_buffer[KBD_BUFFER_SIZE];
static int kbd_read_idx = 0;
static int kbd_write_idx = 0;
static int is_extended = 0;
static int extended_age = 0;

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

static void kbd_buffer_put(char c) {
    int next = (kbd_write_idx + 1) % KBD_BUFFER_SIZE;
    if (next == kbd_read_idx) {
        // Ring buffer full: drop the oldest byte to keep the newest input.
        kbd_read_idx = (kbd_read_idx + 1) % KBD_BUFFER_SIZE;
    }
    kbd_buffer[kbd_write_idx] = c;
    kbd_write_idx = next;
}

char keyboard_getchar(void) {
    if (kbd_read_idx == kbd_write_idx) return 0;
    
    char c = kbd_buffer[kbd_read_idx];
    kbd_read_idx = (kbd_read_idx + 1) % KBD_BUFFER_SIZE;
    return c;
}

int keyboard_has_input(void) {
    return kbd_read_idx != kbd_write_idx;
}

void keyboard_inject_char(char c) {
    if (c) {
        kbd_buffer_put(c);
    }
}

void keyboard_handler(void) {
    uint8_t status = inb(KBD_STATUS_PORT);
    if (!(status & 0x01)) {
        return;
    }

    // Auxiliary (mouse) byte: drain it so it is not read as a scancode.
    if (status & 0x20) {
        (void)inb(KBD_DATA_PORT);
        return;
    }

    uint8_t scancode = inb(KBD_DATA_PORT);
// 0xE0 = extended key prefix
    if (scancode == 0xE0) {
        is_extended = 1;
        extended_age = 0;
        return;
    }

    // Abandon a prefix whose follow-up scancode never arrived.
    if (is_extended && extended_age >= KBD_EXTENDED_MAX_AGE) {
        is_extended = 0;
    }
    extended_age++;
    
    // Key release (break code: bit 7 set)
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

    // Caps lock toggle
    if (scancode == 0x3A) {
        caps_lock = !caps_lock;
        return;
    }
    
    char c = 0;
    if (is_extended) {
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

void keyboard_poll(void) {
    int safety = 64;
    while (safety-- > 0) {
        uint8_t status = inb(KBD_STATUS_PORT);
        if (!(status & 0x01)) break;
        // Bit 5 set means this byte belongs to the mouse; leave it in place.
        if (status & 0x20) break;
        keyboard_handler();
    }
}

void keyboard_init(void) {
    kbd_read_idx = 0;
    kbd_write_idx = 0;
    is_extended = 0;
    shift_pressed = 0;
    caps_lock = 0;

    for (int i = 0; i < KBD_DRAIN_LIMIT; i++) {
        if (!(inb(KBD_STATUS_PORT) & 0x01)) break;
        (void)inb(KBD_DATA_PORT);
    }

    // Unmask IRQ1 (keyboard) in PIC
    uint8_t mask = inb(PIC1_DATA);
    mask &= ~(1 << 1);
    outb(PIC1_DATA, mask);
}

// TODO: Add configurable keyboard layouts (US/UK/DE) selectable at runtime.
// TODO: Implement key-repeat timing control for held keys.
