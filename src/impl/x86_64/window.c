#include "../../intf/window.h"
#include "../../intf/graphics.h"
#include "../../intf/ui.h"

window_t* windows[MAX_WINDOWS];
int window_count = 0;

void init_windowing() {
    for (int i = 0; i < MAX_WINDOWS; i++) {
        windows[i] = 0;
    }
}

window_t* create_window(int x, int y, int width, int height, char* title) {
    if (window_count >= MAX_WINDOWS) {
        return 0; // Max windows reached
    }

    // Find a free spot
    int window_slot = -1;
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i] == 0) {
            window_slot = i;
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

    // Draw window frame
    vga_fill_rect(win->x, win->y, win->width, win->height, COLOR_DARK_GREY);
    vga_draw_rect(win->x, win->y, win->width, win->height, COLOR_WHITE);

    // Draw title bar
    vga_fill_rect(win->x, win->y, win->width, 20, COLOR_BLUE);
    // draw_string - this function is in ui.c and not accessible here yet
    // For now, we'll skip drawing the title
}

void move_window(window_t* win, int new_x, int new_y) {
    if (!win) return;

    win->x = new_x;
    win->y = new_y;
}