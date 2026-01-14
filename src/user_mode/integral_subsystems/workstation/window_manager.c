#include "window_manager.h"

// Forward declarations for window system functions
extern void init_windowing(void);
extern window_t* create_window(int x, int y, int width, int height, char* title);
extern window_t* windows[MAX_WINDOWS];
extern int window_count;

// Window manager state
static uint32_t next_window_id = 1;
static uint32_t active_window_id = 0;

// Window manager functions
void window_manager_init(void) {
    init_windowing(); // Initialize the existing windowing system
    next_window_id = 1;
    active_window_id = 0;
}

void window_manager_shutdown(void) {
    // Cleanup windows
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i]) {
            // TODO: Proper cleanup
        }
    }
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
            // TODO: Store mapping from window_id to window index
            return window_id;
        }
    }

    return 0;
}

void window_destroy(uint32_t window_id) {
    // TODO: Find window by ID and destroy it
}

void window_show(uint32_t window_id) {
    // TODO: Find window and show it
}

void window_hide(uint32_t window_id) {
    // TODO: Find window and hide it
}

void window_move(uint32_t window_id, int x, int y) {
    // TODO: Find window and move it
    // For now, this is a stub
}

void window_resize(uint32_t window_id, uint32_t width, uint32_t height) {
    // TODO: Find window and resize it
}

void window_set_focus(uint32_t window_id) {
    active_window_id = window_id;
    // TODO: Update window focus
}

void window_minimize(uint32_t window_id) {
    // TODO: Minimize window
}

void window_maximize(uint32_t window_id) {
    // TODO: Maximize window
}

void window_restore(uint32_t window_id) {
    // TODO: Restore window
}

void window_bring_to_front(uint32_t window_id) {
    // TODO: Change z-order
}

void window_send_to_back(uint32_t window_id) {
    // TODO: Change z-order
}

uint32_t window_get_count(void) {
    return window_count;
}

uint32_t window_get_active(void) {
    return active_window_id;
}

uint32_t window_get_at_position(int x, int y) {
    // TODO: Find window at position
    return 0;
}