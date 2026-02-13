#include "window_manager.h"

// Forward declarations for window system functions
extern void init_windowing(void);
extern window_t* create_window(int x, int y, int width, int height, char* title);
extern window_t* windows[MAX_WINDOWS];
extern int window_count;

// Window manager state
static uint32_t next_window_id = 1;
static uint32_t active_window_id = 0;
static uint32_t id_to_slot[MAX_WINDOWS];
static int32_t saved_x[MAX_WINDOWS];
static int32_t saved_y[MAX_WINDOWS];
static uint32_t saved_w[MAX_WINDOWS];
static uint32_t saved_h[MAX_WINDOWS];
static uint8_t minimized[MAX_WINDOWS];
static uint8_t maximized[MAX_WINDOWS];

static int find_slot_by_id(uint32_t window_id) {
    if (window_id == 0) return -1;
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (id_to_slot[i] == window_id && windows[i]) {
            return i;
        }
    }
    return -1;
}

// Window manager functions
void window_manager_init(void) {
    init_windowing(); // Initialize the existing windowing system
    next_window_id = 1;
    active_window_id = 0;
    for (int i = 0; i < MAX_WINDOWS; i++) {
        id_to_slot[i] = 0;
        saved_x[i] = 0;
        saved_y[i] = 0;
        saved_w[i] = 0;
        saved_h[i] = 0;
        minimized[i] = 0;
        maximized[i] = 0;
    }
}

void window_manager_shutdown(void) {
    // Cleanup windows
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i]) {
            windows[i] = 0;
            id_to_slot[i] = 0;
        }
    }
    active_window_id = 0;
}

uint32_t window_create(int x, int y, uint32_t width, uint32_t height, const char* title) {
    window_t* win = create_window(x, y, (int)width, (int)height, (char*)title);
    if (!win) {
        return 0; // Failed to create window
    }

    // Find the window index
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i] == win) {
            uint32_t window_id = next_window_id++;
            id_to_slot[i] = window_id;
            minimized[i] = 0;
            maximized[i] = 0;
            saved_x[i] = win->x;
            saved_y[i] = win->y;
            saved_w[i] = (uint32_t)win->width;
            saved_h[i] = (uint32_t)win->height;
            active_window_id = window_id;
            return window_id;
        }
    }

    return 0;
}

void window_destroy(uint32_t window_id) {
    int slot = find_slot_by_id(window_id);
    if (slot < 0) return;
    windows[slot] = 0;
    id_to_slot[slot] = 0;
    minimized[slot] = 0;
    maximized[slot] = 0;
    if (active_window_id == window_id) active_window_id = 0;
    if (window_count > 0) window_count--;
}

void window_show(uint32_t window_id) {
    int slot = find_slot_by_id(window_id);
    if (slot < 0 || !windows[slot]) return;
    windows[slot]->is_active = 1;
    minimized[slot] = 0;
    active_window_id = window_id;
}

void window_hide(uint32_t window_id) {
    int slot = find_slot_by_id(window_id);
    if (slot < 0 || !windows[slot]) return;
    windows[slot]->is_active = 0;
    if (active_window_id == window_id) active_window_id = 0;
}

void window_move(uint32_t window_id, int x, int y) {
    int slot = find_slot_by_id(window_id);
    if (slot < 0 || !windows[slot]) return;
    windows[slot]->x = x;
    windows[slot]->y = y;
}

void window_resize(uint32_t window_id, uint32_t width, uint32_t height) {
    int slot = find_slot_by_id(window_id);
    if (slot < 0 || !windows[slot]) return;
    windows[slot]->width = (int)width;
    windows[slot]->height = (int)height;
}

void window_set_focus(uint32_t window_id) {
    int slot = find_slot_by_id(window_id);
    if (slot < 0 || !windows[slot]) return;
    active_window_id = window_id;
    windows[slot]->is_active = 1;
}

void window_minimize(uint32_t window_id) {
    int slot = find_slot_by_id(window_id);
    if (slot < 0 || !windows[slot]) return;
    minimized[slot] = 1;
    windows[slot]->is_active = 0;
    if (active_window_id == window_id) active_window_id = 0;
}

void window_maximize(uint32_t window_id) {
    int slot = find_slot_by_id(window_id);
    if (slot < 0 || !windows[slot] || maximized[slot]) return;
    saved_x[slot] = windows[slot]->x;
    saved_y[slot] = windows[slot]->y;
    saved_w[slot] = (uint32_t)windows[slot]->width;
    saved_h[slot] = (uint32_t)windows[slot]->height;
    windows[slot]->x = 0;
    windows[slot]->y = 0;
    windows[slot]->width = 320;
    windows[slot]->height = 172; // Keep room for taskbar.
    maximized[slot] = 1;
    minimized[slot] = 0;
    windows[slot]->is_active = 1;
    active_window_id = window_id;
}

void window_restore(uint32_t window_id) {
    int slot = find_slot_by_id(window_id);
    if (slot < 0 || !windows[slot]) return;
    if (maximized[slot]) {
        windows[slot]->x = saved_x[slot];
        windows[slot]->y = saved_y[slot];
        windows[slot]->width = (int)saved_w[slot];
        windows[slot]->height = (int)saved_h[slot];
    }
    maximized[slot] = 0;
    minimized[slot] = 0;
    windows[slot]->is_active = 1;
    active_window_id = window_id;
}

void window_bring_to_front(uint32_t window_id) {
    window_set_focus(window_id);
}

void window_send_to_back(uint32_t window_id) {
    int slot = find_slot_by_id(window_id);
    if (slot < 0 || !windows[slot]) return;
    windows[slot]->is_active = 0;
    if (active_window_id == window_id) active_window_id = 0;
}

uint32_t window_get_count(void) {
    return window_count;
}

uint32_t window_get_active(void) {
    return active_window_id;
}

uint32_t window_get_at_position(int x, int y) {
    for (int i = MAX_WINDOWS - 1; i >= 0; i--) {
        if (windows[i] && windows[i]->is_active) {
            int wx = windows[i]->x;
            int wy = windows[i]->y;
            int ww = windows[i]->width;
            int wh = windows[i]->height;
            if (x >= wx && x < wx + ww && y >= wy && y < wy + wh) {
                return id_to_slot[i];
            }
        }
    }
    return 0;
}

// TODO: Implement window event handling (mouse, keyboard)
// TODO: Add window clipping and content scrolling
// TODO: Implement window transparency and alpha blending
// TODO: Add support for window decorations (icons, buttons)
