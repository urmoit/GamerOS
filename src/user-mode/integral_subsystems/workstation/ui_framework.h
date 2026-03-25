#ifndef UI_FRAMEWORK_H
#define UI_FRAMEWORK_H

#include <stdint.h>

// UI Framework functions
void ui_framework_init(void);
void ui_framework_shutdown(void);
void ui_process_events(void);
void ui_render_frame(void);
void ui_handle_mouse_event(int x, int y, uint8_t button, uint8_t pressed);
void ui_handle_keyboard_event(uint8_t key, uint8_t pressed);
void ui_set_cursor_position(int x, int y);
void ui_show_cursor(void);
void ui_hide_cursor(void);

// UI state
uint8_t ui_is_initialized(void);
uint8_t ui_has_focus(void);

#endif