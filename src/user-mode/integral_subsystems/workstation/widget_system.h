#ifndef WIDGET_SYSTEM_H
#define WIDGET_SYSTEM_H

#include <ui_widgets.h>

// Widget System functions
void widget_system_init(void);
void widget_system_shutdown(void);

// Widget creation (wrappers around existing functions)
ui_widget_t* widget_create_button(int32_t x, int32_t y, uint32_t width, uint32_t height, const char* text);
ui_widget_t* widget_create_label(int32_t x, int32_t y, uint32_t width, uint32_t height, const char* text);
ui_widget_t* widget_create_textbox(int32_t x, int32_t y, uint32_t width, uint32_t height, const char* initial_text);
ui_container_t* widget_create_window(int32_t x, int32_t y, uint32_t width, uint32_t height, const char* title);
ui_container_t* widget_create_panel(int32_t x, int32_t y, uint32_t width, uint32_t height);

// Widget management
void widget_add_to_container(ui_container_t* container, ui_widget_t* widget);
void widget_remove_from_container(ui_container_t* container, ui_widget_t* widget);
void widget_set_position(ui_widget_t* widget, int32_t x, int32_t y);
void widget_set_size(ui_widget_t* widget, uint32_t width, uint32_t height);
void widget_set_text(ui_widget_t* widget, const char* text);
void widget_set_visible(ui_widget_t* widget, uint8_t visible);
void widget_destroy(ui_widget_t* widget);

// Rendering
void widget_render_all(ui_container_t* root_container);

// Event handling
void widget_process_mouse_event(ui_widget_t* widget, int32_t x, int32_t y, uint8_t button, uint8_t pressed);
void widget_process_keyboard_event(ui_widget_t* widget, uint8_t key, uint8_t pressed);

#endif