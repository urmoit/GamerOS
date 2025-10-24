#include "vga_graphics.h"
#include "x86_64/port.h"

// VGA Graphics Mode 13h (320x200, 256 colors)
#define VGA_WIDTH 320
#define VGA_HEIGHT 200
#define VGA_MEMORY 0xA0000

// VGA Ports
#define VGA_CRTC_INDEX 0x3D4
#define VGA_CRTC_DATA 0x3D5
#define VGA_MISC_OUTPUT 0x3C2
#define VGA_SEQ_INDEX 0x3C4
#define VGA_SEQ_DATA 0x3C5
#define VGA_GC_INDEX 0x3CE
#define VGA_GC_DATA 0x3CF
#define VGA_AC_INDEX 0x3C0
#define VGA_AC_WRITE 0x3C0
#define VGA_AC_READ 0x3C1

// Windows 11 Color Palette
#define W11_BLACK 0x00
#define W11_WHITE 0x0F
#define W11_BLUE 0x01
#define W11_GREEN 0x02
#define W11_RED 0x04
#define W11_CYAN 0x03
#define W11_MAGENTA 0x05
#define W11_YELLOW 0x0E
#define W11_GRAY 0x08
#define W11_LIGHT_GRAY 0x07
#define W11_DARK_BLUE 0x01
#define W11_LIGHT_BLUE 0x09

// Windows 11 specific colors
#define W11_TASKBAR_COLOR 0x08      // Dark gray
#define W11_WINDOW_COLOR 0x0F       // White
#define W11_BORDER_COLOR 0x08       // Dark gray
#define W11_TITLE_COLOR 0x01        // Blue
#define W11_BACKGROUND_COLOR 0x00   // Black

static unsigned char* vga_memory = (unsigned char*)VGA_MEMORY;

void vga_set_mode_13h() {
    // Set VGA mode 13h (320x200, 256 colors)
    asm volatile (
        "mov $0x13, %%ax\n"
        "int $0x10\n"
        :
        :
        : "ax"
    );
}

void vga_clear_screen(unsigned char color) {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_memory[i] = color;
    }
}

void vga_put_pixel(int x, int y, unsigned char color) {
    if (x >= 0 && x < VGA_WIDTH && y >= 0 && y < VGA_HEIGHT) {
        vga_memory[y * VGA_WIDTH + x] = color;
    }
}

void vga_draw_rectangle(int x, int y, int width, int height, unsigned char color) {
    for (int dy = 0; dy < height; dy++) {
        for (int dx = 0; dx < width; dx++) {
            vga_put_pixel(x + dx, y + dy, color);
        }
    }
}

void vga_draw_border(int x, int y, int width, int height, unsigned char color) {
    // Top and bottom borders
    for (int dx = 0; dx < width; dx++) {
        vga_put_pixel(x + dx, y, color);
        vga_put_pixel(x + dx, y + height - 1, color);
    }
    // Left and right borders
    for (int dy = 0; dy < height; dy++) {
        vga_put_pixel(x, y + dy, color);
        vga_put_pixel(x + width - 1, y + dy, color);
    }
}

void vga_draw_char(int x, int y, char c, unsigned char color) {
    // Simple 8x8 character rendering
    // This is a basic implementation - in a real OS you'd use a font bitmap
    if (c >= 'A' && c <= 'Z') {
        // Draw a simple 'A' pattern for demonstration
        for (int dy = 0; dy < 8; dy++) {
            for (int dx = 0; dx < 8; dx++) {
                if ((dy == 0) || (dy == 4) || (dx == 0) || (dx == 7)) {
                    vga_put_pixel(x + dx, y + dy, color);
                }
            }
        }
    } else if (c >= '0' && c <= '9') {
        // Draw a simple number pattern
        for (int dy = 0; dy < 8; dy++) {
            for (int dx = 0; dx < 8; dx++) {
                if ((dy == 0) || (dy == 7) || (dx == 0) || (dx == 7)) {
                    vga_put_pixel(x + dx, y + dy, color);
                }
            }
        }
    }
}

void vga_draw_string(int x, int y, const char* str, unsigned char color) {
    int pos_x = x;
    while (*str) {
        vga_draw_char(pos_x, y, *str, color);
        pos_x += 8;
        str++;
    }
}

// Windows 11 Desktop Functions
void w11_draw_desktop() {
    // Clear screen with Windows 11 background
    vga_clear_screen(W11_BACKGROUND_COLOR);
    
    // Draw Windows 11 wallpaper pattern (simplified)
    for (int y = 0; y < VGA_HEIGHT; y += 20) {
        for (int x = 0; x < VGA_WIDTH; x += 20) {
            vga_put_pixel(x, y, W11_DARK_BLUE);
        }
    }
}

void w11_draw_taskbar() {
    int taskbar_height = 30;
    int taskbar_y = VGA_HEIGHT - taskbar_height;
    
    // Draw taskbar background
    vga_draw_rectangle(0, taskbar_y, VGA_WIDTH, taskbar_height, W11_TASKBAR_COLOR);
    
    // Draw Start button
    vga_draw_rectangle(5, taskbar_y + 5, 60, 20, W11_LIGHT_BLUE);
    vga_draw_string(10, taskbar_y + 10, "Start", W11_WHITE);
    
    // Draw time in taskbar
    vga_draw_string(VGA_WIDTH - 50, taskbar_y + 10, "12:34", W11_WHITE);
}

void w11_draw_window(int x, int y, int width, int height, const char* title) {
    // Draw window border
    vga_draw_border(x, y, width, height, W11_BORDER_COLOR);
    
    // Draw title bar
    vga_draw_rectangle(x + 1, y + 1, width - 2, 20, W11_TITLE_COLOR);
    vga_draw_string(x + 5, y + 8, title, W11_WHITE);
    
    // Draw window content area
    vga_draw_rectangle(x + 1, y + 21, width - 2, height - 22, W11_WINDOW_COLOR);
    
    // Draw close button (X)
    vga_draw_rectangle(x + width - 20, y + 3, 15, 15, W11_RED);
    vga_draw_string(x + width - 17, y + 8, "X", W11_WHITE);
}

void w11_draw_desktop_icons() {
    // Draw "This PC" icon
    vga_draw_rectangle(20, 50, 60, 40, W11_LIGHT_GRAY);
    vga_draw_string(25, 70, "This PC", W11_BLACK);
    
    // Draw "Recycle Bin" icon
    vga_draw_rectangle(100, 50, 60, 40, W11_LIGHT_GRAY);
    vga_draw_string(105, 70, "Recycle", W11_BLACK);
}
