#include "graphics.h"
#include "framebuffer.h"

extern framebuffer_info_t framebuffer_info;

static unsigned int* framebuffer;
static unsigned int width;
static unsigned int height;
static unsigned int pitch;

void graphics_init() {
    framebuffer = (unsigned int*)framebuffer_info.addr;
    width = framebuffer_info.width;
    height = framebuffer_info.height;
    pitch = framebuffer_info.pitch;
}

void clear_screen(unsigned int color) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            put_pixel(x, y, color);
        }
    }
}

void put_pixel(int x, int y, unsigned int color) {
    if (x >= 0 && x < width && y >= 0 && y < height) {
        framebuffer[y * (pitch / 4) + x] = color;
    }
}

void draw_rectangle(int x, int y, int w, int h, unsigned int color) {
    for (int dy = 0; dy < h; dy++) {
        for (int dx = 0; dx < w; dx++) {
            put_pixel(x + dx, y + dy, color);
        }
    }
}

void draw_border(int x, int y, int w, int h, unsigned int color) {
    // Top and bottom borders
    for (int dx = 0; dx < w; dx++) {
        put_pixel(x + dx, y, color);
        put_pixel(x + dx, y + h - 1, color);
    }
    // Left and right borders
    for (int dy = 0; dy < h; dy++) {
        put_pixel(x, y + dy, color);
        put_pixel(x + w - 1, y + dy, color);
    }
}

// Dummy function for now
void draw_char(int x, int y, char c, unsigned int color) {
    //
}

void draw_string(int x, int y, const char* str, unsigned int color) {
    int pos_x = x;
    while (*str) {
        draw_char(pos_x, y, *str, color);
        pos_x += 8;
        str++;
    }
}

#define W11_BLACK 0x000000
#define W11_WHITE 0xFFFFFF
#define W11_BLUE 0x0000FF
#define W11_GREEN 0x00FF00
#define W11_RED 0xFF0000
#define W11_CYAN 0x00FFFF
#define W11_MAGENTA 0xFF00FF
#define W11_YELLOW 0xFFFF00
#define W11_GRAY 0x808080
#define W11_LIGHT_GRAY 0xD3D3D3
#define W11_DARK_BLUE 0x00008B
#define W11_LIGHT_BLUE 0xADD8E6

// Windows 11 specific colors
#define W11_TASKBAR_COLOR 0x333333      // Dark gray
#define W11_WINDOW_COLOR 0xFFFFFF       // White
#define W11_BORDER_COLOR 0x808080       // Dark gray
#define W11_TITLE_COLOR 0x0000FF        // Blue
#define W11_BACKGROUND_COLOR 0x000000   // Black


// Windows 11 Desktop Functions
void w11_draw_desktop() {
    // Clear screen with Windows 11 background
    clear_screen(W11_BACKGROUND_COLOR);
    
    // Draw Windows 11 wallpaper pattern (simplified)
    for (int y = 0; y < height; y += 20) {
        for (int x = 0; x < width; x += 20) {
            put_pixel(x, y, W11_DARK_BLUE);
        }
    }
}

void w11_draw_taskbar() {
    int taskbar_height = 30;
    int taskbar_y = height - taskbar_height;
    
    // Draw taskbar background
    draw_rectangle(0, taskbar_y, width, taskbar_height, W11_TASKBAR_COLOR);
    
    // Draw Start button
    draw_rectangle(5, taskbar_y + 5, 60, 20, W11_LIGHT_BLUE);
    draw_string(10, taskbar_y + 10, "Start", W11_WHITE);
    
    // Draw time in taskbar
    draw_string(width - 50, taskbar_y + 10, "12:34", W11_WHITE);
}

void w11_draw_window(int x, int y, int w, int h, const char* title) {
    // Draw window border
    draw_border(x, y, w, h, W11_BORDER_COLOR);
    
    // Draw title bar
    draw_rectangle(x + 1, y + 1, w - 2, 20, W11_TITLE_COLOR);
    draw_string(x + 5, y + 8, title, W11_WHITE);
    
    // Draw window content area
    draw_rectangle(x + 1, y + 21, w - 2, h - 22, W11_WINDOW_COLOR);
    
    // Draw close button (X)
    draw_rectangle(x + w - 20, y + 3, 15, 15, W11_RED);
    draw_string(x + w - 17, y + 8, "X", W11_WHITE);
}

void w11_draw_desktop_icons() {
    // Draw "This PC" icon
    draw_rectangle(20, 50, 60, 40, W11_LIGHT_GRAY);
    draw_string(25, 70, "This PC", W11_BLACK);
    
    // Draw "Recycle Bin" icon
    draw_rectangle(100, 50, 60, 40, W11_LIGHT_GRAY);
    draw_string(105, 70, "Recycle", W11_BLACK);
}
