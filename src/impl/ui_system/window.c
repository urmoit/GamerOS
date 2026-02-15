#include "../../intf/window.h"
#include "../../intf/graphics.h"
#include "../../intf/ui.h"

window_t* windows[MAX_WINDOWS];
int window_count = 0;
static int focused_window_slot = -1;

void set_window_focus(window_t* win);
void bring_window_to_front(window_t* win);

static int find_window_index(window_t* win) {
    if (!win) return -1;
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i] == win) return i;
    }
    return -1;
}

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

    // Use static window pool instead of kmalloc
    static window_t window_pool[MAX_WINDOWS];
    window_t* new_window = &window_pool[window_slot];
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
    set_window_focus(new_window);
    bring_window_to_front(new_window);

    return new_window;
}

void draw_window(window_t* win) {
    if (!win) return;

    // Define local size constants
    #define TITLE_BAR_HEIGHT 20

    // Draw window frame
    vga_fill_rect(win->x, win->y, win->width, win->height, COLOR_DARK_GREY);
    vga_draw_rect(win->x, win->y, win->width, win->height, COLOR_WHITE);

    // Draw title bar
    uint8_t title_color = win->is_active ? COLOR_BLUE : COLOR_DARK_GREY;
    vga_fill_rect(win->x, win->y, win->width, TITLE_BAR_HEIGHT, title_color);

    // Draw window title using draw_string from ui.c
    if (win->title) {
        draw_string(win->x + 5, win->y + 5, win->title, COLOR_WHITE);
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

void resize_window(window_t* win, int new_width, int new_height) {
    if (!win) return;

    #define MIN_WINDOW_WIDTH  80
    #define MIN_WINDOW_HEIGHT 60

    if (new_width < MIN_WINDOW_WIDTH) new_width = MIN_WINDOW_WIDTH;
    if (new_height < MIN_WINDOW_HEIGHT) new_height = MIN_WINDOW_HEIGHT;
    if (new_width > (int)current_width) new_width = (int)current_width;
    if (new_height > (int)current_height) new_height = (int)current_height;
    if (win->x + new_width > (int)current_width) new_width = (int)current_width - win->x;
    if (win->y + new_height > (int)current_height) new_height = (int)current_height - win->y;
    if (new_width < MIN_WINDOW_WIDTH) new_width = MIN_WINDOW_WIDTH;
    if (new_height < MIN_WINDOW_HEIGHT) new_height = MIN_WINDOW_HEIGHT;

    win->width = new_width;
    win->height = new_height;
}

void set_window_focus(window_t* win) {
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i]) windows[i]->is_active = 0;
    }
    if (!win) {
        focused_window_slot = -1;
        return;
    }
    int idx = find_window_index(win);
    if (idx >= 0) {
        windows[idx]->is_active = 1;
        focused_window_slot = idx;
    }
}

void bring_window_to_front(window_t* win) {
    int idx = find_window_index(win);
    if (idx < 0) return;

    int target = idx;
    while (target + 1 < MAX_WINDOWS && windows[target + 1]) {
        target++;
    }
    if (target == idx) return;

    window_t* moved = windows[idx];
    for (int i = idx; i < target; i++) {
        windows[i] = windows[i + 1];
    }
    windows[target] = moved;

    if (focused_window_slot == idx) focused_window_slot = target;
    else if (focused_window_slot > idx && focused_window_slot <= target) focused_window_slot--;
}

window_t* get_focused_window(void) {
    if (focused_window_slot < 0 || focused_window_slot >= MAX_WINDOWS) return 0;
    return windows[focused_window_slot];
}

// TODO: Implement window minimization and maximization
// TODO: Add window close and minimize buttons
// TODO: Implement drag and drop for window movement
