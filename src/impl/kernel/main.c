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

void process1_entry() {
    for(;;) {
        // Process 1 does something
        for (int i = 0; i < 1000000; i++) { __asm__("nop"); }
    }
}

void process2_entry() {
    for(;;) {
        // Process 2 does something else
        for (int i = 0; i < 1000000; i++) { __asm__("nop"); }
    }
}

void kernel_main(void) {
    // Initialize VGA graphics mode 13h (320x200x256)
    vga_init_mode13();

    // Set the desktop background
    vga_set_desktop_background();

    // Initialize UI system and windowing
    ui_init();
    init_windowing();
    fs_init();
    mm_init();
    scheduler_init();
    keyboard_init();
    mouse_init();

    // Allocate some memory for testing
    char* test_mem = (char*)kmalloc(100);
    if (test_mem) {
        // For now, just allocate and don't use
    }

    // Create some sample windows
    create_window(50, 50, 100, 80, "Window 1");
    create_window(80, 70, 120, 90, "Window 2");

    // Create a sample file
    file_t* test_file = fs_create_file("test.txt");
    if (test_file) {
        fs_write_file(test_file, (const uint8_t*)"Hello, GamerOS!", 16);
    }

    // Create sample processes
    create_process(process1_entry);
    create_process(process2_entry);

    // Draw header with tabs
    ui_draw_header();

    // Draw the taskbar
    ui_draw_taskbar();

    // Draw the start menu
    ui_draw_start_menu();

    // Hang - in a real OS, you'd have a scheduler and event loop here
    for(;;) {
        // Draw all open windows
        for (int i = 0; i < window_count; i++) {
            draw_window(windows[i]);
        }

        // Toggle the start menu for testing purposes
        ui_toggle_start_menu();
        ui_draw_start_menu();
        ui_draw_taskbar(); // Redraw taskbar to update clock and window list
        
        // Schedule next process
        schedule();

        // A simple delay
        for (int i = 0; i < 100000000; i++) {
            __asm__("nop");
        }
    }
}

