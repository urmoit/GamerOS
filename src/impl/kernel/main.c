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
    // Print that we reached kernel_main
    const char* msg = "Kernel main reached!";
    char* video_memory = (char*)0xB8000;
    for (int i = 0; msg[i] != '\0'; i++) {
        video_memory[160 + i * 2] = msg[i];     // Character
        video_memory[160 + i * 2 + 1] = 0x0A;  // Green on black
    }

    // Add debug output before any hardware initialization
    const char* debug_msg = "Starting hardware init...";
    for (int i = 0; debug_msg[i] != '\0'; i++) {
        video_memory[320 + i * 2] = debug_msg[i];     // Character
        video_memory[320 + i * 2 + 1] = 0x0E;        // Yellow on black
    }

    // Try graphics initialization - should work now with VGA mode 13h
    if (vga_set_mode(VGA_MODE_13H)) {
        graphics_initialized = 1;

        // Set desktop background
        vga_set_desktop_background();

        // Draw welcome message using custom font
        vga_draw_string(50, 50, "Welcome to GamerOS!", 0x0F); // White text
        vga_draw_string(50, 70, "Custom font system loaded successfully", 0x0A); // Green text
        vga_draw_string(50, 90, "Graphics mode active", 0x0B); // Cyan text

        const char* graphics_msg = "Graphics mode initialized successfully";
        for (int i = 0; graphics_msg[i] != '\0'; i++) {
            video_memory[320 + i * 2] = graphics_msg[i];     // Character
            video_memory[320 + i * 2 + 1] = 0x0A;           // Green on black
        }
    } else {
        graphics_initialized = 0; // Fallback to text mode

        const char* text_msg = "Graphics initialization failed, using text mode";
        for (int i = 0; text_msg[i] != '\0'; i++) {
            video_memory[320 + i * 2] = text_msg[i];     // Character
            video_memory[320 + i * 2 + 1] = 0x0E;       // Yellow on black
        }
    }

    // Initialize UI system and windowing (only if graphics work)
    if (graphics_initialized) {
        ui_init();
        init_windowing();
    }

    // Add debug output after graphics init
    const char* init_msg = "Graphics init done, starting other hardware...";
    for (int i = 0; init_msg[i] != '\0'; i++) {
        video_memory[640 + i * 2] = init_msg[i];     // Character
        video_memory[640 + i * 2 + 1] = 0x0B;       // Cyan on black
    }

    fs_init();
    mm_init();
    scheduler_init();

    // Add debug output after basic init
    const char* basic_msg = "Basic init done, starting peripherals...";
    for (int i = 0; basic_msg[i] != '\0'; i++) {
        video_memory[800 + i * 2] = basic_msg[i];     // Character
        video_memory[800 + i * 2 + 1] = 0x0B;        // Cyan on black
    }

    keyboard_init();
    mouse_init();

    // Allocate some memory for testing
    char* test_mem = (char*)kmalloc(100);
    if (test_mem) {
        // For now, just allocate and don't use
    }

    if (graphics_initialized) {
        // Create some sample windows with better positioning for 320x200 resolution
        create_window(50, 120, 150, 100, "Window 1");
        create_window(220, 130, 180, 120, "Window 2");

        // Demonstrate new graphics capabilities
        // Draw some shapes using the new primitives
        vga_draw_circle(100, 50, 20, rgb_to_color(255, 0, 0, 255)); // Red circle
        vga_draw_line(150, 40, 200, 60, rgb_to_color(0, 255, 0, 255)); // Green line
        vga_fill_triangle(250, 40, 280, 20, 310, 60, rgb_to_color(0, 0, 255, 128)); // Blue triangle with alpha

        // Create a software render buffer for advanced effects
        render_buffer_t* render_buf = create_render_buffer(80, 80);
        if (render_buf) {
            clear_render_buffer(render_buf, rgb_to_color(255, 255, 0, 200)); // Yellow with alpha
            draw_circle_software(render_buf, 40, 40, 25, rgb_to_color(255, 0, 255, 255)); // Magenta circle
            render_buffer_to_screen(render_buf, 20, 150); // Render to screen
            destroy_render_buffer(render_buf);
        }
    }

    // Create a sample file
    file_t* test_file = fs_create_file("test.txt");
    if (test_file) {
        fs_write_file(test_file, (const uint8_t*)"Hello, GamerOS!", 16);
    }

    // Add debug output before process creation
    const char* proc_msg = "Creating processes...";
    for (int i = 0; proc_msg[i] != '\0'; i++) {
        video_memory[960 + i * 2] = proc_msg[i];     // Character
        video_memory[960 + i * 2 + 1] = 0x0D;       // Magenta on black
    }

    // Create sample processes
    create_process(process1_entry);
    create_process(process2_entry);

   // Only draw UI elements if graphics mode is available
   if (graphics_initialized) {
       // Draw header with tabs
       ui_draw_header();

       // Draw the taskbar
       ui_draw_taskbar();

       // Draw the start menu
       ui_draw_start_menu();
   }

    // Print status message in text mode
    const char* status_msg = graphics_initialized ?
        "GamerOS running in graphics mode" :
        "GamerOS running in text mode (graphics not available)";
    for (int i = 0; status_msg[i] != '\0'; i++) {
        video_memory[480 + i * 2] = status_msg[i];     // Character
        video_memory[480 + i * 2 + 1] = graphics_initialized ? 0x0A : 0x0E;  // Green or yellow
    }

    // Add debug output before main loop
    const char* loop_msg = "Entering main loop...";
    for (int i = 0; loop_msg[i] != '\0'; i++) {
        video_memory[1120 + i * 2] = loop_msg[i];     // Character
        video_memory[1120 + i * 2 + 1] = 0x0F;       // White on black
    }

    // Hang - in a real OS, you'd have a scheduler and event loop here
    for(;;) {
        if (graphics_initialized) {
            // Draw all open windows
            for (int i = 0; i < window_count; i++) {
                draw_window(windows[i]);
            }

            // Toggle the start menu for testing purposes
            ui_toggle_start_menu();
            ui_draw_start_menu();
            ui_draw_taskbar(); // Redraw taskbar to update clock and window list
        }

        // Schedule next process
        schedule();

        // A simple delay - use a timer interrupt in a real OS
        for (volatile int i = 0; i < 1000000; i++) {
            // Reduced delay and made i volatile to prevent optimization
        }
    }
}

