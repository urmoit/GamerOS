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
#include "../../intf/scheduler.h"
#include "../../intf/ui_widgets.h"


void process1_entry() {
    int counter = 0;
    for(;;) {
        // Process 1: Simple counter task
        counter++;
        if (counter % 100000 == 0) {
            // Yield control to other processes periodically
            // __asm__("int $32"); // Temporarily disable software interrupts
        }
        __asm__("nop");

        // Add exit condition to prevent infinite loop
        if (counter >= 10000000) { // Exit after 10 million iterations
            extern void terminate_process(int);
            terminate_process(0); // Terminate this process cleanly
            break;
        }
    }
}

void process2_entry() {
    int counter = 0;
    for(;;) {
        // Process 2: Different counter task
        counter--;
        if (counter % 150000 == 0) {
            // Yield control to other processes periodically
            // __asm__("int $32"); // Temporarily disable software interrupts
        }
        __asm__("nop");

        // Add exit condition to prevent infinite loop
        if (counter <= -10000000) { // Exit after 10 million iterations
            extern void terminate_process(int);
            terminate_process(1); // Terminate this process cleanly
            break;
        }
    }
}

void kernel_main(void) {
    // Test mm_init
    mm_init();

    // Test vga_set_mode
    vga_set_mode(VGA_MODE_13H);

    // Test idt_init
    idt_init();

    // Unmask timer interrupt (IRQ0) for scheduling
    outb(0x21, 0xFE);

    // Test init_windowing
    init_windowing();

    // Test ui_init
    ui_init();

    // Initialize UI widgets system
    ui_widgets_init();

    // Initialize scheduler
    scheduler_init();

    // Create GUI application process
    create_process(gui_app_entry);

    // Enable interrupts
    __asm__("sti");

    // Main kernel loop - let GUI application handle display
    for(;;) {
        // Yield control to allow GUI application to run
        __asm__("nop");
    }
}

