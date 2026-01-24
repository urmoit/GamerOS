// src/impl/kernel/main.c - ULTRA-SIMPLIFIED TEST VERSION

#include "../../intf/kernel.h"
#include "../../intf/stdint.h"
#include "../../intf/graphics.h"

void kernel_main(void) {
    // Direct framebuffer access - no functions
    volatile uint8_t* fb = (volatile uint8_t*)0xA0000;

    // Fill entire screen with different colors in bands to test
    // This bypasses ALL graphics functions to isolate the issue

    // Band 1: Black (0-39)
    for (uint32_t y = 0; y < 40; y++) {
        for (uint32_t x = 0; x < 320; x++) {
            fb[y * 320 + x] = 0x00;
        }
    }

    // Band 2: Blue (40-79)
    for (uint32_t y = 40; y < 80; y++) {
        for (uint32_t x = 0; x < 320; x++) {
            fb[y * 320 + x] = 0x01;
        }
    }

    // Band 3: Green (80-119)
    for (uint32_t y = 80; y < 120; y++) {
        for (uint32_t x = 0; x < 320; x++) {
            fb[y * 320 + x] = 0x02;
        }
    }

    // Band 4: Red (120-159)
    for (uint32_t y = 120; y < 160; y++) {
        for (uint32_t x = 0; x < 320; x++) {
            fb[y * 320 + x] = 0x04;
        }
    }

    // Band 5: White (160-199)
    for (uint32_t y = 160; y < 200; y++) {
        for (uint32_t x = 0; x < 320; x++) {
            fb[y * 320 + x] = 0x0F;
        }
    }

    // Memory barrier to ensure all writes complete
    __asm__ volatile("mfence" ::: "memory");

    // Now try initializing graphics properly
    vga_set_mode(VGA_MODE_13H);

    // Draw some test patterns using graphics functions
    vga_fill_rect(10, 10, 50, 30, 0x0C);  // Bright red box
    vga_fill_rect(70, 10, 50, 30, 0x0A);  // Bright green box
    vga_fill_rect(130, 10, 50, 30, 0x09); // Bright blue box

    // Draw text
    vga_draw_string(10, 50, "HELLO GAMEROS!", 0x0E);

    // Initialize rest of system
    extern void microkernel_init();
    extern void executive_init();
    extern void user_mode_init();

    microkernel_init();
    executive_init();
    user_mode_init();

    // Simple loop - no GUI for now
    while (1) {
        __asm__("hlt");
    }
}

// TODO: Add proper kernel initialization sequence with error handling
// TODO: Implement preemptive multitasking and scheduler
// TODO: Add memory management and virtual memory support
// TODO: Implement system call interface and user mode switching