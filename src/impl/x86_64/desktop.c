#include "desktop.h"
#include "graphics.h"
#include "mouse.h"

// Desktop state
static int desktop_initialized = 0;
static int start_menu_open = 0;
static int window_count = 0;

// Window structures
typedef struct {
    int x, y, width, height;
    char title[32];
    int visible;
} Window;

static Window windows[10];  // Support up to 10 windows

void desktop_init() {
    if (desktop_initialized) return;
    
    // Initialize graphics
    graphics_init();
    
    // Initialize desktop
    w11_draw_desktop();
    w11_draw_taskbar();
    w11_draw_desktop_icons();
    
    // Initialize mouse
    mouse_set_position(160, 100);
    mouse_show();
    
    desktop_initialized = 1;
}

void desktop_update() {
    if (!desktop_initialized) return;
    
    // Redraw desktop elements
    w11_draw_desktop();
    w11_draw_taskbar();
    w11_draw_desktop_icons();
    
    // Redraw all windows
    for (int i = 0; i < window_count; i++) {
        if (windows[i].visible) {
            w11_draw_window(windows[i].x, windows[i].y, 
                          windows[i].width, windows[i].height, 
                          windows[i].title);
        }
    }
    
    // Draw mouse cursor
    mouse_draw_cursor();
}

void desktop_handle_click() {
    if (!desktop_initialized) return;
    
    // Check if clicking on Start button
    if (mouse_is_over_start_button()) {
        start_menu_open = !start_menu_open;
        if (start_menu_open) {
            // Draw start menu
            w11_draw_start_menu();
        } else {
            // Close start menu
            desktop_update();
        }
    }
}

void w11_draw_start_menu() {
    // Draw start menu background
    draw_rectangle(5, 140, 200, 30, 0xD3D3D3);
    draw_border(5, 140, 200, 30, 0x808080);
    
    // Draw start menu items
    draw_string(10, 150, "File Explorer", 0x000000);
    draw_string(10, 160, "Settings", 0x000000);
    draw_string(10, 170, "Power", 0x000000);
}

int desktop_create_window(int x, int y, int width, int height, const char* title) {
    if (window_count >= 10) return -1;  // Too many windows
    
    windows[window_count].x = x;
    windows[window_count].y = y;
    windows[window_count].width = width;
    windows[window_count].height = height;
    windows[window_count].visible = 1;
    
    // Copy title
    int i = 0;
    while (title[i] && i < 31) {
        windows[window_count].title[i] = title[i];
        i++;
    }
    windows[window_count].title[i] = '\0';
    
    window_count++;
    
    // Redraw desktop
    desktop_update();
    
    return window_count - 1;
}

void desktop_close_window(int window_id) {
    if (window_id >= 0 && window_id < window_count) {
        windows[window_id].visible = 0;
        desktop_update();
    }
}

void desktop_move_window(int window_id, int new_x, int new_y) {
    if (window_id >= 0 && window_id < window_count && windows[window_id].visible) {
        windows[window_id].x = new_x;
        windows[window_id].y = new_y;
        desktop_update();
    }
}

// Windows 11 specific functions
void w11_show_welcome() {
    // Draw Windows 11 welcome screen
    draw_rectangle(50, 50, 220, 100, 0xFFFFFF);
    draw_border(50, 50, 220, 100, 0x808080);
    
    // Title bar
    draw_rectangle(51, 51, 218, 20, 0x0000FF);
    draw_string(60, 60, "Welcome to Gameros", 0xFFFFFF);
    
    // Content
    draw_string(60, 80, "Your custom Windows 11-like OS", 0x000000);
    draw_string(60, 90, "Click Start to begin", 0x000000);
    draw_string(60, 100, "Move mouse to interact", 0x000000);
    draw_string(60, 110, "Press any key to continue", 0x000000);
}

void w11_draw_system_tray() {
    // Draw system tray in taskbar
    int tray_x = 250;
    int tray_y = 175;
    
    // Battery icon
    draw_rectangle(tray_x, tray_y + 5, 15, 10, 0x00FF00);
    draw_border(tray_x, tray_y + 5, 15, 10, 0x808080);
    
    // Network icon
    draw_rectangle(tray_x + 20, tray_y + 5, 15, 10, 0x0000FF);
    draw_border(tray_x + 20, tray_y + 5, 15, 10, 0x808080);
    
    // Volume icon
    draw_rectangle(tray_x + 40, tray_y + 5, 15, 10, 0xFFFF00);
    draw_border(tray_x + 40, tray_y + 5, 15, 10, 0x808080);
}
