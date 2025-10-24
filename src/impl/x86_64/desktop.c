#include "desktop.h"
#include "vga_graphics.h"
#include "mouse.h"

// Windows 11 Color Constants (from vga_graphics.c)
#define W11_LIGHT_GRAY 0x07
#define W11_BORDER_COLOR 0x08
#define W11_BLACK 0x00
#define W11_WINDOW_COLOR 0x0F
#define W11_TITLE_COLOR 0x01
#define W11_WHITE 0x0F
#define W11_GREEN 0x02
#define W11_BLUE 0x01
#define W11_YELLOW 0x0E

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
    
    // Set VGA graphics mode
    vga_set_mode_13h();
    
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
    vga_draw_rectangle(5, 140, 200, 30, W11_LIGHT_GRAY);
    vga_draw_border(5, 140, 200, 30, W11_BORDER_COLOR);
    
    // Draw start menu items
    vga_draw_string(10, 150, "File Explorer", W11_BLACK);
    vga_draw_string(10, 160, "Settings", W11_BLACK);
    vga_draw_string(10, 170, "Power", W11_BLACK);
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
    vga_draw_rectangle(50, 50, 220, 100, W11_WINDOW_COLOR);
    vga_draw_border(50, 50, 220, 100, W11_BORDER_COLOR);
    
    // Title bar
    vga_draw_rectangle(51, 51, 218, 20, W11_TITLE_COLOR);
    vga_draw_string(60, 60, "Welcome to Gameros", W11_WHITE);
    
    // Content
    vga_draw_string(60, 80, "Your custom Windows 11-like OS", W11_BLACK);
    vga_draw_string(60, 90, "Click Start to begin", W11_BLACK);
    vga_draw_string(60, 100, "Move mouse to interact", W11_BLACK);
    vga_draw_string(60, 110, "Press any key to continue", W11_BLACK);
}

void w11_draw_system_tray() {
    // Draw system tray in taskbar
    int tray_x = 250;
    int tray_y = 175;
    
    // Battery icon
    vga_draw_rectangle(tray_x, tray_y + 5, 15, 10, W11_GREEN);
    vga_draw_border(tray_x, tray_y + 5, 15, 10, W11_BORDER_COLOR);
    
    // Network icon
    vga_draw_rectangle(tray_x + 20, tray_y + 5, 15, 10, W11_BLUE);
    vga_draw_border(tray_x + 20, tray_y + 5, 15, 10, W11_BORDER_COLOR);
    
    // Volume icon
    vga_draw_rectangle(tray_x + 40, tray_y + 5, 15, 10, W11_YELLOW);
    vga_draw_border(tray_x + 40, tray_y + 5, 15, 10, W11_BORDER_COLOR);
}
