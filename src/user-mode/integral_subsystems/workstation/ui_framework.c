#include "ui_framework.h"
#include <keyboard.h>
#include <mouse.h>
#include <graphics.h>
#include "desktop_manager.h"

// Forward declarations for UI functions
extern void ui_init(void);
extern void ui_draw_header(void);
extern void ui_draw_taskbar(void);
extern void ui_draw_clock(void);
extern void ui_draw_start_menu(void);

// UI Framework state
static uint8_t ui_initialized = 0;
static uint8_t ui_has_focus_flag = 1;
static uint8_t cursor_visible = 1;
static int cursor_x = 0;
static int cursor_y = 0;
static uint8_t last_mouse_buttons = 0;

typedef struct {
    uint8_t type; // 1=mouse, 2=keyboard
    int x;
    int y;
    uint8_t a;
    uint8_t b;
} ui_event_internal_t;

#define UI_EVENT_QUEUE_SIZE 64
static ui_event_internal_t event_queue[UI_EVENT_QUEUE_SIZE];
static uint32_t event_head = 0;
static uint32_t event_tail = 0;

static int enqueue_event(const ui_event_internal_t* ev) {
    uint32_t next = (event_head + 1) % UI_EVENT_QUEUE_SIZE;
    if (next == event_tail) return 0;
    event_queue[event_head] = *ev;
    event_head = next;
    return 1;
}

static int dequeue_event(ui_event_internal_t* ev) {
    if (event_tail == event_head) return 0;
    *ev = event_queue[event_tail];
    event_tail = (event_tail + 1) % UI_EVENT_QUEUE_SIZE;
    return 1;
}

// UI Framework functions
void ui_framework_init(void) {
    ui_init(); // Initialize existing UI system
    ui_initialized = 1;
    ui_has_focus_flag = 1;
    cursor_visible = 1;
    cursor_x = 160; // Center of screen
    cursor_y = 100;
    last_mouse_buttons = 0;
    event_head = 0;
    event_tail = 0;
}

void ui_framework_shutdown(void) {
    ui_initialized = 0;
    event_head = 0;
    event_tail = 0;
}

void ui_process_events(void) {
    if (!ui_initialized) return;

    mouse_state_t mouse = mouse_get_state();
    if (mouse.x != cursor_x || mouse.y != cursor_y || mouse.buttons != last_mouse_buttons) {
        ui_event_internal_t ev = {1, mouse.x, mouse.y, mouse.buttons, (uint8_t)(mouse.buttons != 0)};
        enqueue_event(&ev);
        last_mouse_buttons = mouse.buttons;
    }

    while (keyboard_has_input()) {
        char key = keyboard_getchar();
        ui_event_internal_t ev = {2, 0, 0, (uint8_t)key, 1};
        enqueue_event(&ev);
    }

    ui_event_internal_t ev;
    while (dequeue_event(&ev)) {
        if (ev.type == 1) {
            ui_handle_mouse_event(ev.x, ev.y, ev.a, ev.b);
        } else if (ev.type == 2) {
            ui_handle_keyboard_event(ev.a, ev.b);
        }
    }
}

void ui_render_frame(void) {
    if (!ui_initialized) return;

    desktop_update();
    if (cursor_visible) {
        draw_cursor(cursor_x, cursor_y);
    }
    swap_buffers();
}

void ui_handle_mouse_event(int x, int y, uint8_t button, uint8_t pressed) {
    (void)button;
    (void)pressed;
    cursor_x = x;
    cursor_y = y;
}

void ui_handle_keyboard_event(uint8_t key, uint8_t pressed) {
    (void)pressed;
    // ESC toggles focus for simple testing.
    if (key == 27) {
        ui_has_focus_flag = !ui_has_focus_flag;
    }
}

void ui_set_cursor_position(int x, int y) {
    cursor_x = x;
    cursor_y = y;
}

void ui_show_cursor(void) {
    cursor_visible = 1;
}

void ui_hide_cursor(void) {
    cursor_visible = 0;
}

uint8_t ui_is_initialized(void) {
    return ui_initialized;
}

uint8_t ui_has_focus(void) {
    return ui_has_focus_flag;
}
