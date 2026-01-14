#include "ui_framework.h"

// Forward declarations for UI functions
extern void ui_init(void);
extern void ui_draw_header(void);
extern void ui_draw_taskbar(void);
extern void ui_draw_clock(void);
extern void ui_draw_start_menu(void);

// UI Framework state
static uint8_t ui_initialized = 0;
static uint8_t ui_has_focus_flag = 1;
static int cursor_x = 0;
static int cursor_y = 0;

// UI Framework functions
void ui_framework_init(void) {
    ui_init(); // Initialize existing UI system
    ui_initialized = 1;
    ui_has_focus_flag = 1;
    cursor_x = 160; // Center of screen
    cursor_y = 100;
}

void ui_framework_shutdown(void) {
    ui_initialized = 0;
}

void ui_process_events(void) {
    // TODO: Process pending UI events
    // This would integrate with the input system
}

void ui_render_frame(void) {
    if (!ui_initialized) return;

    // Render desktop
    ui_draw_header();

    // TODO: Render windows
    // TODO: Render widgets

    // Render taskbar and clock
    ui_draw_taskbar();
    ui_draw_clock();

    // Render start menu if open
    ui_draw_start_menu();
}

void ui_handle_mouse_event(int x, int y, uint8_t button, uint8_t pressed) {
    cursor_x = x;
    cursor_y = y;

    // TODO: Handle mouse events for widgets and windows
}

void ui_handle_keyboard_event(uint8_t key, uint8_t pressed) {
    // TODO: Handle keyboard events
}

void ui_set_cursor_position(int x, int y) {
    cursor_x = x;
    cursor_y = y;
}

void ui_show_cursor(void) {
    // TODO: Show mouse cursor
}

void ui_hide_cursor(void) {
    // TODO: Hide mouse cursor
}

uint8_t ui_is_initialized(void) {
    return ui_initialized;
}

uint8_t ui_has_focus(void) {
    return ui_has_focus_flag;
}