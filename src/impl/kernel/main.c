#include "print.h"
#include "keyboard.h"
#include "x86_64/rtc.h"
#include "graphics.h" // Changed from vga_graphics.h
#include "mouse.h"
#include "desktop.h"
#include "x86_64/idt.h"
#include "framebuffer.h" // New include

extern framebuffer_info_t framebuffer_info; // Declare as extern

// Windows 11 Gameros Kernel
void kernel_main() {
    idt_init();

    // Print framebuffer info for debugging
    print_str("Framebuffer Info:\n");
    print_str("  Addr: "); print_uint64_hex(framebuffer_info.addr); print_str("\n");
    print_str("  Width: "); print_uint64_dec(framebuffer_info.width); print_str("\n");
    print_str("  Height: "); print_uint64_dec(framebuffer_info.height); print_str("\n");
    print_str("  Pitch: "); print_uint64_dec(framebuffer_info.pitch); print_str("\n");
    print_str("  BPP: "); print_uint64_dec(framebuffer_info.bpp); print_str("\n");

    // Initialize Windows 11-like desktop
    desktop_init();
    
    // Show welcome screen
    w11_show_welcome();
    
    // Create some sample windows
    desktop_create_window(50, 30, 200, 120, "File Explorer");
    desktop_create_window(100, 60, 180, 100, "Settings");
    
    // Main Windows 11 desktop loop
    uint8_t prev_seconds = 0;
    int frame_count = 0;
    
    while (1) {
        // Update desktop every frame
        desktop_update();
        
        // Handle mouse movement (simulate with keyboard for now)
        // In a real implementation, you'd read actual mouse data
        
        // Update clock every second
        uint8_t seconds = rtc_seconds();
        if (seconds != prev_seconds) {
            frame_count++;
            
            // Every 5 seconds, create a new window
            if (frame_count % 5 == 0) {
                char window_title[32];
                // Simple window title generation
                window_title[0] = 'W';
                window_title[1] = 'i';
                window_title[2] = 'n';
                window_title[3] = 'd';
                window_title[4] = 'o';
                window_title[5] = 'w';
                window_title[6] = ' ';
                window_title[7] = '0' + (frame_count / 5);
                window_title[8] = '\0';
                
                desktop_create_window(30 + (frame_count * 10), 40 + (frame_count * 5), 
                                   150, 80, window_title);
            }
        }
        
        prev_seconds = seconds;
        
        // Simple delay to prevent too fast updates
        for (volatile int i = 0; i < 100000; i++);
    }
}
