#include "widget_system.h"

// Forward declarations for UI widget functions
extern void ui_widgets_init(void);
extern ui_widget_t* ui_create_button(int32_t x, int32_t y, uint32_t width, uint32_t height, const char* text);
extern ui_widget_t* ui_create_label(int32_t x, int32_t y, uint32_t width, uint32_t height, const char* text);
extern ui_widget_t* ui_create_textbox(int32_t x, int32_t y, uint32_t width, uint32_t height, const char* initial_text);
extern ui_container_t* ui_create_window(int32_t x, int32_t y, uint32_t width, uint32_t height, const char* title);
extern ui_container_t* ui_create_panel(int32_t x, int32_t y, uint32_t width, uint32_t height);
extern void ui_add_child(ui_container_t* container, ui_widget_t* widget);
extern void ui_remove_child(ui_container_t* container, ui_widget_t* widget);
extern void ui_set_position(ui_widget_t* widget, int32_t x, int32_t y);
extern void ui_set_size(ui_widget_t* widget, uint32_t width, uint32_t height);
extern void ui_set_text(ui_widget_t* widget, const char* text);
extern void ui_set_visible(ui_widget_t* widget, uint8_t visible);
extern void ui_destroy_widget(ui_widget_t* widget);
extern void ui_render_container(ui_container_t* root_container);
extern void ui_process_mouse_event(ui_widget_t* widget, ui_event_t* event);
extern void ui_process_keyboard_event(ui_widget_t* widget, ui_event_t* event);

// Widget System functions
void widget_system_init(void) {
    ui_widgets_init(); // Initialize existing widget system
}

void widget_system_shutdown(void) {
    // Cleanup widgets
}

// Widget creation (wrappers)
ui_widget_t* widget_create_button(int32_t x, int32_t y, uint32_t width, uint32_t height, const char* text) {
    return ui_create_button(x, y, width, height, text);
}

ui_widget_t* widget_create_label(int32_t x, int32_t y, uint32_t width, uint32_t height, const char* text) {
    return ui_create_label(x, y, width, height, text);
}

ui_widget_t* widget_create_textbox(int32_t x, int32_t y, uint32_t width, uint32_t height, const char* initial_text) {
    return ui_create_textbox(x, y, width, height, initial_text);
}

ui_container_t* widget_create_window(int32_t x, int32_t y, uint32_t width, uint32_t height, const char* title) {
    return ui_create_window(x, y, width, height, title);
}

ui_container_t* widget_create_panel(int32_t x, int32_t y, uint32_t width, uint32_t height) {
    return ui_create_panel(x, y, width, height);
}

// Widget management
void widget_add_to_container(ui_container_t* container, ui_widget_t* widget) {
    ui_add_child(container, widget);
}

void widget_remove_from_container(ui_container_t* container, ui_widget_t* widget) {
    ui_remove_child(container, widget);
}

void widget_set_position(ui_widget_t* widget, int32_t x, int32_t y) {
    ui_set_position(widget, x, y);
}

void widget_set_size(ui_widget_t* widget, uint32_t width, uint32_t height) {
    ui_set_size(widget, width, height);
}

void widget_set_text(ui_widget_t* widget, const char* text) {
    ui_set_text(widget, text);
}

void widget_set_visible(ui_widget_t* widget, uint8_t visible) {
    ui_set_visible(widget, visible);
}

void widget_destroy(ui_widget_t* widget) {
    ui_destroy_widget(widget);
}

// Rendering
void widget_render_all(ui_container_t* root_container) {
    ui_render_container(root_container);
}

// Event handling
void widget_process_mouse_event(ui_widget_t* widget, int32_t x, int32_t y, uint8_t button, uint8_t pressed) {
    ui_event_t event;
    if (pressed) {
        event.type = EVENT_MOUSE_DOWN;
    } else {
        event.type = EVENT_MOUSE_UP;
    }
    event.x = x;
    event.y = y;
    event.button = button;

    ui_process_mouse_event(widget, &event);
}

void widget_process_keyboard_event(ui_widget_t* widget, uint8_t key, uint8_t pressed) {
    ui_event_t event;
    if (pressed) {
        event.type = EVENT_KEY_DOWN;
        event.key_code = key;
    } else {
        event.type = EVENT_KEY_UP;
        event.key_code = key;
    }

    ui_process_keyboard_event(widget, &event);
}