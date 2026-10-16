#ifndef UI_WIDGETS_H
#define UI_WIDGETS_H

#include "stdint.h"
#include "graphics.h"

struct ui_widget_t;
struct ui_container_t;

typedef enum {
    WIDGET_BUTTON,
    WIDGET_LABEL,
    WIDGET_TEXTBOX,
    WIDGET_CHECKBOX,
    WIDGET_SCROLLBAR,
    WIDGET_PROGRESS_BAR,
    WIDGET_SLIDER,
    WIDGET_COMBOBOX,
    WIDGET_LISTBOX,
    WIDGET_MENU,
    WIDGET_MENU_ITEM,
    WIDGET_WINDOW,
    WIDGET_PANEL,
    WIDGET_TAB_CONTROL,
    WIDGET_TAB_PAGE
} widget_type_t;

typedef enum {
    STATE_NORMAL,
    STATE_HOVER,
    STATE_PRESSED,
    STATE_DISABLED,
    STATE_FOCUSED,
    STATE_SELECTED
} widget_state_t;

typedef enum {
    EVENT_MOUSE_ENTER,
    EVENT_MOUSE_LEAVE,
    EVENT_MOUSE_DOWN,
    EVENT_MOUSE_UP,
    EVENT_MOUSE_CLICK,
    EVENT_MOUSE_DOUBLE_CLICK,
    EVENT_KEY_DOWN,
    EVENT_KEY_UP,
    EVENT_KEY_PRESS,
    EVENT_FOCUS_GAIN,
    EVENT_FOCUS_LOSE,
    EVENT_VALUE_CHANGED,
    EVENT_SELECTION_CHANGED
} event_type_t;

typedef struct {
    event_type_t type;
    uint32_t x, y;
    uint32_t button;
    uint32_t key_code;
    char key_char;
    void* data;
} ui_event_t;

typedef void (*event_handler_t)(struct ui_widget_t* widget, ui_event_t* event);

typedef struct ui_widget_t {
    widget_type_t type;
    widget_state_t state;

    int32_t x, y;
    uint32_t width, height;

    uint32_t background_color;
    uint32_t foreground_color;
    uint32_t border_color;
    uint8_t border_width;
    uint32_t shadow_color;
    uint8_t shadow_offset;

    char* text;
    uint8_t text_size;
    uint32_t text_color;

    uint8_t visible : 1;
    uint8_t enabled : 1;
    uint8_t focused : 1;
    uint8_t hovered : 1;
    uint8_t pressed : 1;

    struct ui_container_t* parent;

    event_handler_t on_mouse_enter;
    event_handler_t on_mouse_leave;
    event_handler_t on_mouse_down;
    event_handler_t on_mouse_up;
    event_handler_t on_mouse_click;
    event_handler_t on_key_down;
    event_handler_t on_key_up;
    event_handler_t on_focus_gain;
    event_handler_t on_focus_lose;
    event_handler_t on_value_changed;

    union {
        struct {
            uint8_t is_default;
            uint8_t is_cancel;
        } button;

        struct {
            char* buffer;
            uint32_t buffer_size;
            uint32_t cursor_pos;
            uint32_t selection_start;
            uint32_t selection_end;
            uint8_t multiline;
            uint8_t readonly;
        } textbox;

        struct {
            uint8_t checked;
            char* label;
        } checkbox;

        struct {
            uint32_t min_value;
            uint32_t max_value;
            uint32_t current_value;
            uint32_t page_size;
            uint8_t orientation; // 0=horizontal, 1=vertical
        } scrollbar;

        struct {
            uint32_t min_value;
            uint32_t max_value;
            uint32_t current_value;
            uint8_t show_text;
        } progress_bar;

        struct {
            uint32_t min_value;
            uint32_t max_value;
            uint32_t current_value;
            uint8_t orientation; // 0=horizontal, 1=vertical
        } slider;

        struct {
            char** items;
            uint32_t item_count;
            int32_t selected_index;
            uint32_t visible_items;
            uint32_t scroll_offset;
        } listbox;

        struct {
            struct ui_widget_t** items;
            uint32_t item_count;
            uint8_t is_open;
        } menu;
    } data;

    void* user_data;
} ui_widget_t;

typedef struct ui_container_t {
    ui_widget_t base;

    ui_widget_t** children;
    uint32_t child_count;
    uint32_t child_capacity;

    uint8_t auto_layout;
    uint32_t padding_left, padding_right, padding_top, padding_bottom;
    uint32_t spacing_horizontal, spacing_vertical;

    uint8_t scrollable;
    uint32_t scroll_x, scroll_y;
    uint32_t content_width, content_height;
} ui_container_t;

typedef struct {
    uint32_t primary_color;
    uint32_t secondary_color;
    uint32_t accent_color;
    uint32_t background_color;
    uint32_t surface_color;
    uint32_t text_color;
    uint32_t text_secondary_color;
    uint32_t border_color;
    uint32_t shadow_color;

    uint32_t button_normal_bg;
    uint32_t button_hover_bg;
    uint32_t button_pressed_bg;
    uint32_t button_disabled_bg;
    uint32_t button_border_color;

    uint32_t textbox_bg;
    uint32_t textbox_border_color;
    uint32_t textbox_focus_border_color;

    uint32_t window_bg;
    uint32_t window_border_color;
    uint32_t window_title_bg;
    uint32_t window_title_text_color;

    uint8_t font_size;
    uint8_t border_radius;
    uint8_t shadow_offset;
} ui_theme_t;

ui_widget_t* ui_create_button(int32_t x, int32_t y, uint32_t width, uint32_t height, const char* text);
ui_widget_t* ui_create_label(int32_t x, int32_t y, uint32_t width, uint32_t height, const char* text);
ui_widget_t* ui_create_textbox(int32_t x, int32_t y, uint32_t width, uint32_t height, const char* initial_text);
ui_widget_t* ui_create_checkbox(int32_t x, int32_t y, uint32_t width, uint32_t height, const char* label, uint8_t checked);
ui_widget_t* ui_create_scrollbar(int32_t x, int32_t y, uint32_t width, uint32_t height, uint8_t orientation);
ui_widget_t* ui_create_progress_bar(int32_t x, int32_t y, uint32_t width, uint32_t height);
ui_widget_t* ui_create_slider(int32_t x, int32_t y, uint32_t width, uint32_t height, uint8_t orientation);
ui_widget_t* ui_create_listbox(int32_t x, int32_t y, uint32_t width, uint32_t height);
ui_widget_t* ui_create_menu(int32_t x, int32_t y, uint32_t width, uint32_t height);
ui_container_t* ui_create_window(int32_t x, int32_t y, uint32_t width, uint32_t height, const char* title);
ui_container_t* ui_create_panel(int32_t x, int32_t y, uint32_t width, uint32_t height);

void ui_destroy_widget(ui_widget_t* widget);
void ui_add_child(ui_container_t* parent, ui_widget_t* child);
void ui_remove_child(ui_container_t* parent, ui_widget_t* child);

void ui_set_position(ui_widget_t* widget, int32_t x, int32_t y);
void ui_set_size(ui_widget_t* widget, uint32_t width, uint32_t height);
void ui_set_text(ui_widget_t* widget, const char* text);
void ui_set_visible(ui_widget_t* widget, uint8_t visible);
void ui_set_enabled(ui_widget_t* widget, uint8_t enabled);
void ui_set_focused(ui_widget_t* widget, uint8_t focused);

void ui_process_mouse_event(ui_widget_t* widget, ui_event_t* event);
void ui_process_keyboard_event(ui_widget_t* widget, ui_event_t* event);
void ui_update_widget_state(ui_widget_t* widget, uint32_t mouse_x, uint32_t mouse_y, uint8_t mouse_button_pressed);

void ui_render_widget(ui_widget_t* widget);
void ui_render_container(ui_container_t* container);

extern ui_theme_t current_theme;
void ui_set_theme(ui_theme_t* theme);
void ui_load_default_theme();

uint8_t ui_point_in_widget(ui_widget_t* widget, int32_t x, int32_t y);
ui_widget_t* ui_find_widget_at(ui_container_t* container, int32_t x, int32_t y);

void ui_widgets_init();

#endif