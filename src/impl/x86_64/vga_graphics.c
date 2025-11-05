#include "../../intf/graphics.h"
#include "../../intf/ui.h"

// VGA Mode 13h framebuffer
static uint8_t* vga_framebuffer = (uint8_t*)0xA0000;

void vga_init_mode13(void) {
    // Switch to VGA mode 13h (320x200x256)
    __asm__ volatile (
        "mov $0x13, %%ah\n"
        "int $0x10\n"
        :
        :
        : "ah"
    );
}

void vga_set_pixel(uint32_t x, uint32_t y, uint8_t color) {
    if (x >= VGA_WIDTH || y >= VGA_HEIGHT) {
        return;
    }
    vga_framebuffer[y * VGA_WIDTH + x] = color;
}

uint8_t vga_get_pixel(uint32_t x, uint32_t y) {
    if (x >= VGA_WIDTH || y >= VGA_HEIGHT) {
        return 0;
    }
    return vga_framebuffer[y * VGA_WIDTH + x];
}

void vga_fill_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint8_t color) {
    for (uint32_t py = y; py < y + height && py < VGA_HEIGHT; py++) {
        for (uint32_t px = x; px < x + width && px < VGA_WIDTH; px++) {
            vga_set_pixel(px, py, color);
        }
    }
}

void vga_draw_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint8_t color) {
    for (uint32_t i = 0; i < width; i++) {
        vga_set_pixel(x + i, y, color);
        vga_set_pixel(x + i, y + height - 1, color);
    }
    for (uint32_t i = 0; i < height; i++) {
        vga_set_pixel(x, y + i, color);
        vga_set_pixel(x + width - 1, y + i, color);
    }
}

void vga_set_desktop_background(void) {
    // Create a simple gradient background
    for (uint32_t y = 0; y < VGA_HEIGHT; y++) {
        for (uint32_t x = 0; x < VGA_WIDTH; x++) {
            // A simple gradient from blue to black
            vga_set_pixel(x, y, (uint8_t)(COLOR_BLUE + (y / 16)));
        }
    }
}

void vga_clear(uint8_t color) {
    for (uint32_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_framebuffer[i] = color;
    }
}

