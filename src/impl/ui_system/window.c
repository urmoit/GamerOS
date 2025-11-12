#include "../../intf/window.h"
#include "../../intf/graphics.h"
#include "../../intf/ui.h"

window_t* windows[MAX_WINDOWS];
int window_count = 0;

void init_windowing() {
    for (size_t i = 0; i < MAX_WINDOWS; i++) {
        windows[i] = 0;
    }
}

window_t* create_window(int x, int y, int width, int height, char* title) {
    if (window_count >= MAX_WINDOWS || !title) {
        return 0; // Max windows reached or invalid title
    }

    // Find a free spot
    int window_slot = -1;
    for (size_t i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i] == 0) {
            window_slot = (int)i;
            break;
        }
    }

    if (window_slot == -1) return 0; // Should not happen if window_count is correct

    // Allocate memory for the window
    window_t* new_window = (window_t*)kmalloc(sizeof(window_t));
    if (!new_window) {
        return 0; // Memory allocation failed
    }

    new_window->x = x;
    new_window->y = y;
    new_window->width = width;
    new_window->height = height;
    new_window->title = title;
    new_window->is_active = 1;

    windows[window_slot] = new_window;
    window_count++;

    return new_window;
}

void draw_window(window_t* win) {
    if (!win) return;

    // Define color constants
    #define COLOR_DARK_GREY 7
    #define COLOR_WHITE 15
    #define COLOR_BLUE 1
    #define TITLE_BAR_HEIGHT 20

    // Draw window frame
    vga_fill_rect(win->x, win->y, win->width, win->height, COLOR_DARK_GREY);
    vga_draw_rect(win->x, win->y, win->width, win->height, COLOR_WHITE);

    // Draw title bar
    vga_fill_rect(win->x, win->y, win->width, TITLE_BAR_HEIGHT, COLOR_BLUE);

    // Draw window title using draw_string from ui.c
    if (win->title) {
        draw_string((uint32_t)(win->x + 5), (uint32_t)(win->y + 5), win->title, COLOR_WHITE);
    }
}

void move_window(window_t* win, int new_x, int new_y) {
    if (!win) return;

    // Define constants for magic numbers
    #define VGA_WIDTH_CONSTANT  320
    #define VGA_HEIGHT_CONSTANT 200
    #define MIN_WINDOW_DIMENSION 50

    // Validate coordinates are within reasonable bounds
    if (new_x < 0) new_x = 0;
    if (new_y < 0) new_y = 0;
    if (new_x > VGA_WIDTH_CONSTANT - MIN_WINDOW_DIMENSION) new_x = VGA_WIDTH_CONSTANT - MIN_WINDOW_DIMENSION;
    if (new_y > VGA_HEIGHT_CONSTANT - MIN_WINDOW_DIMENSION) new_y = VGA_HEIGHT_CONSTANT - MIN_WINDOW_DIMENSION;

    win->x = new_x;
    win->y = new_y;
}