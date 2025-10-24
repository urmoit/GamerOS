#include "mouse.h"
#include "x86_64/port.h"
#include "graphics.h" // Changed from vga_graphics.h

// PS/2 Mouse Commands
#define MOUSE_CMD_RESET 0xFF
#define MOUSE_CMD_ENABLE 0xF4
#define MOUSE_CMD_DISABLE 0xF5
#define MOUSE_CMD_SET_SAMPLE_RATE 0xF3
#define MOUSE_CMD_SET_RESOLUTION 0xE8

// Mouse Data
static int mouse_x = 160;  // Center of screen
static int mouse_y = 100;
static int mouse_buttons = 0;
static int mouse_visible = 1;

// Mouse cursor drawing
void mouse_draw_cursor() {
    if (!mouse_visible) return;
    
    // Draw simple arrow cursor
    for (int i = 0; i < 8; i++) {
        put_pixel(mouse_x + i, mouse_y, 0x0F);  // White
        put_pixel(mouse_x, mouse_y + i, 0x0F);  // White
    }
    
    // Draw cursor border (black outline)
    for (int i = 0; i < 8; i++) {
        put_pixel(mouse_x + i - 1, mouse_y - 1, 0x00);  // Black
        put_pixel(mouse_x - 1, mouse_y + i - 1, 0x00);  // Black
    }
}

void mouse_erase_cursor() {
    // Simple cursor erasing - in a real implementation you'd save/restore background
    // For now, we'll just redraw the desktop
    w11_draw_desktop();
    w11_draw_taskbar();
    w11_draw_desktop_icons();
}

void mouse_move(int delta_x, int delta_y) {
    mouse_erase_cursor();
    
    mouse_x += delta_x;
    mouse_y += delta_y;
    
    // Keep cursor within screen bounds
    // These values should be dynamic based on framebuffer info
    if (mouse_x < 0) mouse_x = 0;
    if (mouse_x >= 320) mouse_x = 319; // TODO: Use actual screen width
    if (mouse_y < 0) mouse_y = 0;
    if (mouse_y >= 200) mouse_y = 199; // TODO: Use actual screen height
    
    mouse_draw_cursor();
}

void mouse_set_position(int x, int y) {
    mouse_erase_cursor();
    
    mouse_x = x;
    mouse_y = y;
    
    // Keep cursor within screen bounds
    // These values should be dynamic based on framebuffer info
    if (mouse_x < 0) mouse_x = 0;
    if (mouse_x >= 320) mouse_x = 319; // TODO: Use actual screen width
    if (mouse_y < 0) mouse_y = 0;
    if (mouse_y >= 200) mouse_y = 199; // TODO: Use actual screen height
    
    mouse_draw_cursor();
}

void mouse_set_buttons(int buttons) {
    mouse_buttons = buttons;
}

int mouse_get_x() {
    return mouse_x;
}

int mouse_get_y() {
    return mouse_y;
}

int mouse_get_buttons() {
    return mouse_buttons;
}

void mouse_show() {
    mouse_visible = 1;
    mouse_draw_cursor();
}

void mouse_hide() {
    mouse_visible = 0;
    mouse_erase_cursor();
}

// Simple mouse click detection for Windows 11 elements
// These values should be dynamic based on framebuffer info
int mouse_is_over_start_button() {
    return (mouse_x >= 5 && mouse_x <= 65 && mouse_y >= 175 && mouse_y <= 195); // TODO: Use actual screen height
}

int mouse_is_over_window(int x, int y, int width, int height) {
    return (mouse_x >= x && mouse_x <= x + width && mouse_y >= y && mouse_y <= y + height);
}
