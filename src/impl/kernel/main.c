#include "print.h"
#include "keyboard.h"
#include "x86_64/rtc.h"
#include "vga_graphics.h"
#include "mouse.h"
#include "desktop.h"
#include "x86_64/idt.h"

// Windows 11 Gameros Kernel
void kernel_main() {
    idt_init();
    print_str("Kernel started. Graphics disabled.");
    while(1);
}
