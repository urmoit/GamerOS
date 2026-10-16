#include <multiboot.h>
#include <graphics.h>
#include <string.h>
#include <stdint.h>

// Minimal kernel entry point
void kernel_main(multiboot_info_t* mbi, uint32_t magic) {
    uint16_t width = 1920;
    uint16_t height = 1080;
    uint8_t bpp = 32;
    
    if (mbi && (mbi->flags & MULTIBOOT_INFO_FRAMEBUFFER_INFO)) {
        width = mbi->framebuffer_width;
        height = mbi->framebuffer_height;
        bpp = mbi->framebuffer_bpp;
    }
    
    graphics_init(width, height, bpp);
    
    if (graphics_is_truecolor()) {
        vga_fill_rect(0, 0, width, height, 0xFF1F1F1F);  // Fluent dark bg
    } else {
        vga_fill_rect(0, 0, width, height, 0x00);
    }
    
    draw_string(50, 50, "GamerOS 1.500 booting...", 0xFFE4E4E4);
    
    for (;;) {
        __asm__ volatile("hlt");
    }
}

void kmalloc() {}
void kfree() {}
