#include <multiboot.h>
#include <graphics.h>
#include <string.h>
#include <stdint.h>

// Minimal kernel entry point
void kernel_main(multiboot_info_t* mbi, uint32_t magic) {
    // Initialize graphics
    uint16_t width = 1920;
    uint16_t height = 1080;
    uint8_t bpp = 32;
    
    // Check for valid framebuffer
    if (mbi && (mbi->flags & MULTIBOOT_INFO_FRAMEBUFFER_INFO)) {
        width = mbi->framebuffer_width;
        height = mbi->framebuffer_height;
        bpp = mbi->framebuffer_bpp;
    }
    
    // Initialize basic graphics
    graphics_init(width, height, bpp);
    
    // Clear screen
    if (graphics_is_truecolor()) {
        vga_fill_rect(0, 0, width, height, 0xFF1F1F1F);  // Fluent dark bg
    } else {
        vga_fill_rect(0, 0, width, height, 0x00);
    }
    
    // Draw boot message
    draw_string(50, 50, "GamerOS 1.400 booting...", 0xFFE4E4E4);
    
    // Halt
    for (;;) {
        __asm__ volatile("hlt");
    }
}

// Dummy functions to satisfy linker
void kmalloc() {}
void kfree() {}
