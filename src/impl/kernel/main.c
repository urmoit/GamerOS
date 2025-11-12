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
    // Simple kernel main - just print a message and loop
    char* video_memory = (char*)0xB8000;

    // Print "Kernel running!" message
    const char* msg = "Kernel running!";
    for (size_t i = 0; msg[i] != '\0'; i++) {
        video_memory[160 + i * 2] = msg[i];
        video_memory[160 + i * 2 + 1] = 0x0A; // Green on black
    }

    // Simple main loop
    for(;;) {
        // Just busy wait - no complex initialization
        for (volatile int i = 0; i < 100000; i++) {
            // Do nothing
        }
    }
}

