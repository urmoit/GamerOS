#include "../../intf/keyboard.h"
#include "../../intf/ports.h"
#include "../../intf/pic.h"

#define KBD_DATA_PORT   0x60
#define KBD_STATUS_PORT 0x64

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

void keyboard_handler() {
    uint8_t status;
    int8_t  scancode;

    // Read keyboard status
    status = inb(KBD_STATUS_PORT);
    // If the lowest bit is set, it means there is data in the output buffer
    if (status & 0x01) {
        scancode = inb(KBD_DATA_PORT);

        if (scancode < 0) return; // Key release

        // Convert scancode to ASCII
        char c = kbd_us[(int)scancode];
        // For now, we'll just store the last character pressed
        // In a real OS, this would go into a buffer
        // For simplicity, we just print chars to screen for debugging
    }

    pic_eoi(1); // End of interrupt for PIC slave
}

void keyboard_init() {
    // Enable keyboard interrupts (IRQ1)
    // This involves setting up the PIC and IDT
    // For now, we assume IDT and PIC are set up to handle IRQ1
}
