#include "../../intf/kernel.h"
#include "../../intf/stdint.h"
#include "../../intf/graphics.h"
#include "../../intf/ui.h"
#include "../../intf/rtc.h"
#include "../../intf/window.h"
#include "../../intf/fs.h"
#include "../../intf/mm.h"
#include "../../intf/scheduler.h"
#include "../../intf/keyboard.h"
#include "../../intf/mouse.h"
#include "../../intf/ports.h"
#include "../../intf/idt.h"
#include "../../intf/pic.h"
#include "../../intf/gui_app.h"
#include "../../intf/ui_widgets.h"
#include "../../intf/microkernel.h"
#include "../../intf/executive.h"
#include "../../user_mode/interfaces/user_mode.h"

// VGA_GRAPHICS_BUFFER is defined in ports.h


// Note: process1_entry and process2_entry functions removed as they were unused
// and only served as example code that wasted memory

void kernel_main(void) {
    // Layered OS Architecture Initialization

    // 1. HAL initialization (already done in boot.asm)

    // Initialize graphics mode FIRST (already set in boot.asm, just configure state)
    // This will initialize the palette
    vga_set_mode(VGA_MODE_13H);
    
    // CRITICAL: Draw colored bars IMMEDIATELY after palette initialization
    // Use volatile pointer and ensure writes are not optimized away
    volatile uint8_t* fb = (volatile uint8_t*)VGA_GRAPHICS_BUFFER;
    
    // Draw four colored bars directly to framebuffer
    // White bar (top 50 rows, rows 0-49) - MUST be visible
    for (uint32_t y = 0; y < 50; y++) {
        for (uint32_t x = 0; x < 320; x++) {
            fb[y * 320 + x] = 0x0F; // White (palette 15)
        }
    }
    
    // Red bar (next 50 rows, rows 50-99)
    for (uint32_t y = 50; y < 100; y++) {
        for (uint32_t x = 0; x < 320; x++) {
            fb[y * 320 + x] = 0x04; // Red (palette 4)
        }
    }
    
    // Green bar (next 50 rows, rows 100-149)
    for (uint32_t y = 100; y < 150; y++) {
        for (uint32_t x = 0; x < 320; x++) {
            fb[y * 320 + x] = 0x02; // Green (palette 2)
        }
    }
    
    // Blue bar (bottom 50 rows, rows 150-199)
    for (uint32_t y = 150; y < 200; y++) {
        for (uint32_t x = 0; x < 320; x++) {
            fb[y * 320 + x] = 0x01; // Blue (palette 1)
        }
    }
    
    // Memory barrier to ensure all writes complete before continuing
    __asm__ volatile("mfence" ::: "memory");
    
    // Additional delay to ensure hardware processes the writes
    for (volatile int i = 0; i < 100000; i++) {
        __asm__ volatile("nop");
    }
    
    // Now draw debug strings (after bars are drawn)
    if (graphics_initialized) {
        vga_draw_string(10, 10, "KERNEL MAIN STARTED", 0x0F);
    }

    // 2. Microkernel initialization (process, memory, IPC)
    microkernel_init();
    vga_draw_string(10, 30, "MICROKERNEL INIT", 0x0F);

    // 3. Executive Layer initialization (Object Manager, all Executive Services)
    executive_init();
    vga_draw_string(10, 40, "EXECUTIVE INIT", 0x0F);

    // 4. User Mode Layer initialization (all subsystems)
    user_mode_init();
    vga_draw_string(10, 50, "USER MODE INIT", 0x0F);

    // Unmask timer interrupt (IRQ0) for scheduling
    outb(0x21, 0xFE);
    vga_draw_string(10, 60, "TIMER UNMASKED", 0x0F);

    // Create GUI application process
    create_process(gui_app_entry);
    vga_draw_string(10, 70, "GUI PROCESS CREATED", 0x0F);

    // Enable interrupts
    __asm__("sti");
    vga_draw_string(10, 80, "INTERRUPTS ENABLED", 0x0F);

    // Main kernel loop - let GUI application handle display
    int kernel_counter = 0; // Initialize counter for activity indicator
    for(;;) {
        // Yield control to allow GUI application to run
        __asm__("nop");

        // Simple kernel activity indicator
        kernel_counter++;
        if (kernel_counter % 1000000 == 0) {
            static int toggle = 0;
            toggle = !toggle;
            if (toggle) {
                vga_draw_string(10, 90, "KERNEL RUNNING *", 0x0F);
            } else {
                vga_draw_string(10, 90, "KERNEL RUNNING  ", 0x0F);
            }
        }
    }
}

