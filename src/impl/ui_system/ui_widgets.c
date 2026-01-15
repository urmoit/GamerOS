#include "../../intf/ui_widgets.h"
#include "../../intf/mm.h"
#include "../../intf/string.h"

// Note: strlen and strcpy are now in string.h/string.c

// Global theme
ui_theme_t current_theme;

// Default theme initialization
void ui_load_default_theme() {
    current_theme.primary_color = 0xFF2196F3;     // Blue
    current_theme.secondary_color = 0xFF757575;   // Grey
    current_theme.accent_color = 0xFFFF9800;      // Orange
    current_theme.background_color = 0xFFE0E0E0;  // Light grey
    current_theme.surface_color = 0xFFFFFFFF;     // White
    current_theme.text_color = 0xFF212121;        // Dark grey
    current_theme.text_secondary_color = 0xFF757575; // Medium grey
    current_theme.border_color = 0xFFBDBDBD;      // Light grey
    current_theme.shadow_color = 0x40000000;      // Semi-transparent black

    // Button styles
    current_theme.button_normal_bg = 0xFFE0E0E0;
    current_theme.button_hover_bg = 0xFFBDBDBD;
    current_theme.button_pressed_bg = 0xFF9E9E9E;
    current_theme.button_disabled_bg = 0xFFBDBDBD;
    current_theme.button_border_color = 0xFF9E9E9E;

    // Textbox styles
    current_theme.textbox_bg = 0xFFFFFFFF;
    current_theme.textbox_border_color = 0xFFBDBDBD;
    current_theme.textbox_focus_border_color = 0xFF2196F3;

    // Window styles
    current_theme.window_bg = 0xFFFFFFFF;
    current_theme.window_border_color = 0xFF9E9E9E;
    current_theme.window_title_bg = 0xFFE0E0E0;
    current_theme.window_title_text_color = 0xFF212121;

    // Other settings
    current_theme.font_size = 8;
    current_theme.border_radius = 3;
    current_theme.shadow_offset = 2;
}

void ui_set_theme(ui_theme_t* theme) {
    if (theme) {
        current_theme = *theme;
    }
}

// Widget creation functions
ui_widget_t* ui_create_widget(widget_type_t type, int32_t x, int32_t y, uint32_t width, uint32_t height) {
    ui_widget_t* widget = (ui_widget_t*)kmalloc(sizeof(ui_widget_t));
    if (!widget) return 0;

    memset(widget, 0, sizeof(ui_widget_t));

    widget->type = type;
    widget->state = STATE_NORMAL;
    widget->x = x;
    widget->y = y;
    widget->width = width;
    widget->height = height;

    // Default properties
    widget->background_color = current_theme.surface_color;
    widget->foreground_color = current_theme.primary_color;
    widget->border_color = current_theme.border_color;
    widget->border_width = 1;
    widget->shadow_color = current_theme.shadow_color;
    widget->shadow_offset = current_theme.shadow_offset;

    widget->text_color = current_theme.text_color;
    widget->text_size = current_theme.font_size;

    widget->visible = 1;
    widget->enabled = 1;
    widget->focused = 0;
    widget->hovered = 0;
    widget->pressed = 0;

    return widget;
}

ui_widget_t* ui_create_button(int32_t x, int32_t y, uint32_t width, uint32_t height, const char* text) {
    ui_widget_t* button = ui_create_widget(WIDGET_BUTTON, x, y, width, height);
    if (!button) return 0;

    if (text) {
        size_t len = strlen(text) + 1;
        button->text = (char*)kmalloc(len);
        if (button->text) {
            strcpy(button->text, text);
        }
    }

    button->background_color = current_theme.button_normal_bg;
    button->border_color = current_theme.button_border_color;

    return button;
}

ui_widget_t* ui_create_label(int32_t x, int32_t y, uint32_t width, uint32_t height, const char* text) {
    ui_widget_t* label = ui_create_widget(WIDGET_LABEL, x, y, width, height);
    if (!label) return 0;

    if (text) {
        size_t len = strlen(text) + 1;
        label->text = (char*)kmalloc(len);
        if (label->text) {
            strcpy(label->text, text);
        }
    }

    label->background_color = 0x00000000; // Transparent
    label->border_width = 0;

    return label;
}

ui_widget_t* ui_create_textbox(int32_t x, int32_t y, uint32_t width, uint32_t height, const char* initial_text) {
    ui_widget_t* textbox = ui_create_widget(WIDGET_TEXTBOX, x, y, width, height);
    if (!textbox) return 0;

    textbox->data.textbox.buffer_size = 256;
    textbox->data.textbox.buffer = (char*)kmalloc(textbox->data.textbox.buffer_size);
    if (textbox->data.textbox.buffer) {
        memset(textbox->data.textbox.buffer, 0, textbox->data.textbox.buffer_size);
        if (initial_text) {
            strcpy(textbox->data.textbox.buffer, initial_text);
        }
    }

    textbox->background_color = current_theme.textbox_bg;
    textbox->border_color = current_theme.textbox_border_color;

    return textbox;
}

ui_widget_t* ui_create_checkbox(int32_t x, int32_t y, uint32_t width, uint32_t height, const char* label, uint8_t checked) {
    ui_widget_t* checkbox = ui_create_widget(WIDGET_CHECKBOX, x, y, width, height);
    if (!checkbox) return 0;

    if (label) {
        size_t len = strlen(label) + 1;
        checkbox->data.checkbox.label = (char*)kmalloc(len);
        if (checkbox->data.checkbox.label) {
            strcpy(checkbox->data.checkbox.label, label);
        }
    }

    checkbox->data.checkbox.checked = checked;
    checkbox->background_color = current_theme.surface_color;

    return checkbox;
}

ui_widget_t* ui_create_scrollbar(int32_t x, int32_t y, uint32_t width, uint32_t height, uint8_t orientation) {
    ui_widget_t* scrollbar = ui_create_widget(WIDGET_SCROLLBAR, x, y, width, height);
    if (!scrollbar) return 0;

    scrollbar->data.scrollbar.min_value = 0;
    scrollbar->data.scrollbar.max_value = 100;
    scrollbar->data.scrollbar.current_value = 0;
    scrollbar->data.scrollbar.page_size = 10;
    scrollbar->data.scrollbar.orientation = orientation;

    scrollbar->background_color = current_theme.surface_color;
    scrollbar->foreground_color = current_theme.secondary_color;

    return scrollbar;
}

ui_widget_t* ui_create_progress_bar(int32_t x, int32_t y, uint32_t width, uint32_t height) {
    ui_widget_t* progress = ui_create_widget(WIDGET_PROGRESS_BAR, x, y, width, height);
    if (!progress) return 0;

    progress->data.progress_bar.min_value = 0;
    progress->data.progress_bar.max_value = 100;
    progress->data.progress_bar.current_value = 0;
    progress->data.progress_bar.show_text = 1;

    progress->background_color = current_theme.surface_color;
    progress->foreground_color = current_theme.primary_color;

    return progress;
}

ui_widget_t* ui_create_slider(int32_t x, int32_t y, uint32_t width, uint32_t height, uint8_t orientation) {
    ui_widget_t* slider = ui_create_widget(WIDGET_SLIDER, x, y, width, height);
    if (!slider) return 0;

    slider->data.slider.min_value = 0;
    slider->data.slider.max_value = 100;
    slider->data.slider.current_value = 0;
    slider->data.slider.orientation = orientation;

    slider->background_color = current_theme.surface_color;
    slider->foreground_color = current_theme.accent_color;

    return slider;
}

ui_container_t* ui_create_container(widget_type_t type, int32_t x, int32_t y, uint32_t width, uint32_t height) {
    ui_container_t* container = (ui_container_t*)kmalloc(sizeof(ui_container_t));
    if (!container) return 0;

    memset(container, 0, sizeof(ui_container_t));

    // Initialize base widget
    container->base.type = type;
    container->base.state = STATE_NORMAL;
    container->base.x = x;
    container->base.y = y;
    container->base.width = width;
    container->base.height = height;

    container->base.background_color = current_theme.surface_color;
    container->base.border_color = current_theme.border_color;
    container->base.border_width = 1;
    container->base.visible = 1;
    container->base.enabled = 1;

    // Container-specific initialization
    container->child_capacity = 16;
    container->children = (ui_widget_t**)kmalloc(sizeof(ui_widget_t*) * container->child_capacity);
    if (!container->children) {
        kfree(container);
        return 0;
    }
    memset(container->children, 0, sizeof(ui_widget_t*) * container->child_capacity);

    container->auto_layout = 1;
    container->padding_left = container->padding_right = container->padding_top = container->padding_bottom = 5;
    container->spacing_horizontal = container->spacing_vertical = 5;

    return container;
}

ui_container_t* ui_create_window(int32_t x, int32_t y, uint32_t width, uint32_t height, const char* title) {
    ui_container_t* window = ui_create_container(WIDGET_WINDOW, x, y, width, height);
    if (!window) return 0;

    if (title) {
        size_t len = strlen(title) + 1;
        window->base.text = (char*)kmalloc(len);
        if (window->base.text) {
            strcpy(window->base.text, title);
        }
    }

    window->base.background_color = current_theme.window_bg;
    window->base.border_color = current_theme.window_border_color;

    return window;
}

ui_container_t* ui_create_panel(int32_t x, int32_t y, uint32_t width, uint32_t height) {
    ui_container_t* panel = ui_create_container(WIDGET_PANEL, x, y, width, height);
    if (!panel) return 0;

    panel->base.background_color = current_theme.surface_color;

    return panel;
}

// Widget management
void ui_destroy_widget(ui_widget_t* widget) {
    if (!widget) return;

    // Free widget-specific data
    if (widget->text) {
        kfree(widget->text);
    }

    switch (widget->type) {
        case WIDGET_TEXTBOX:
            if (widget->data.textbox.buffer) {
                kfree(widget->data.textbox.buffer);
            }
            break;
        case WIDGET_CHECKBOX:
            if (widget->data.checkbox.label) {
                kfree(widget->data.checkbox.label);
            }
            break;
        case WIDGET_LISTBOX:
            if (widget->data.listbox.items) {
                for (uint32_t i = 0; i < widget->data.listbox.item_count; i++) {
                    if (widget->data.listbox.items[i]) {
                        kfree(widget->data.listbox.items[i]);
                    }
                }
                kfree(widget->data.listbox.items);
            }
            break;
        case WIDGET_WINDOW:
        case WIDGET_PANEL:
            // Container cleanup handled separately
            break;
        default:
            break;
    }

    kfree(widget);
}

void ui_add_child(ui_container_t* parent, ui_widget_t* child) {
    if (!parent || !child) return;

    // Expand capacity if needed
    if (parent->child_count >= parent->child_capacity) {
        uint32_t new_capacity = parent->child_capacity * 2;
        ui_widget_t** new_children = (ui_widget_t**)kmalloc(sizeof(ui_widget_t*) * new_capacity);
        if (!new_children) return;

        memcpy(new_children, parent->children, sizeof(ui_widget_t*) * parent->child_count);
        memset(new_children + parent->child_count, 0, sizeof(ui_widget_t*) * (new_capacity - parent->child_count));

        kfree(parent->children);
        parent->children = new_children;
        parent->child_capacity = new_capacity;
    }

    parent->children[parent->child_count++] = child;
    child->parent = parent;
}

void ui_remove_child(ui_container_t* parent, ui_widget_t* child) {
    if (!parent || !child) return;

    for (uint32_t i = 0; i < parent->child_count; i++) {
        if (parent->children[i] == child) {
            // Shift remaining children
            for (uint32_t j = i; j < parent->child_count - 1; j++) {
                parent->children[j] = parent->children[j + 1];
            }
            parent->children[--parent->child_count] = 0;
            child->parent = 0;
            break;
        }
    }
}

// Widget properties
void ui_set_position(ui_widget_t* widget, int32_t x, int32_t y) {
    if (!widget) return;
    widget->x = x;
    widget->y = y;
}

void ui_set_size(ui_widget_t* widget, uint32_t width, uint32_t height) {
    if (!widget) return;
    widget->width = width;
    widget->height = height;
}

void ui_set_text(ui_widget_t* widget, const char* text) {
    if (!widget) return;

    if (widget->text) {
        kfree(widget->text);
        widget->text = 0;
    }

    if (text) {
        size_t len = strlen(text) + 1;
        widget->text = (char*)kmalloc(len);
        if (widget->text) {
            strcpy(widget->text, text);
        }
    }
}

void ui_set_visible(ui_widget_t* widget, uint8_t visible) {
    if (!widget) return;
    widget->visible = visible;
}

void ui_set_enabled(ui_widget_t* widget, uint8_t enabled) {
    if (!widget) return;
    widget->enabled = enabled;
}

void ui_set_focused(ui_widget_t* widget, uint8_t focused) {
    if (!widget) return;
    widget->focused = focused;
}

// Utility functions
uint8_t ui_point_in_widget(ui_widget_t* widget, int32_t x, int32_t y) {
    if (!widget || !widget->visible) return 0;

    return (x >= widget->x && x < widget->x + (int32_t)widget->width &&
            y >= widget->y && y < widget->y + (int32_t)widget->height);
}

ui_widget_t* ui_find_widget_at(ui_container_t* container, int32_t x, int32_t y) {
    if (!container) return 0;

    // Check children in reverse order (top to bottom)
    for (int32_t i = (int32_t)container->child_count - 1; i >= 0; i--) {
        ui_widget_t* child = container->children[i];
        if (child && ui_point_in_widget(child, x, y)) {
            // If it's a container, recursively check its children
            if (child->type == WIDGET_WINDOW || child->type == WIDGET_PANEL) {
                ui_widget_t* found = ui_find_widget_at((ui_container_t*)child, x, y);
                if (found) return found;
            }
            return child;
        }
    }

    // Check container itself
    if (ui_point_in_widget(&container->base, x, y)) {
        return &container->base;
    }

    return 0;
}

// Rendering functions (simplified implementations)
void ui_render_widget(ui_widget_t* widget) {
    if (!widget || !widget->visible) return;

    // Draw shadow if specified
    if (widget->shadow_offset > 0) {
        vga_draw_shadow(widget->x, widget->y, widget->width, widget->height,
                       widget->shadow_color, widget->shadow_offset);
    }

    // Draw background
    if ((widget->background_color >> 24) & 0xFF) { // Not fully transparent
        vga_fill_rounded_rect(widget->x, widget->y, widget->width, widget->height,
                             current_theme.border_radius, widget->background_color);
    }

    // Draw border
    if (widget->border_width > 0) {
        vga_draw_rounded_rect(widget->x, widget->y, widget->width, widget->height,
                             current_theme.border_radius, widget->border_color);
    }

    // Draw text
    if (widget->text) {
        uint32_t text_x = widget->x + widget->border_width + 4;
        uint32_t text_y = widget->y + widget->border_width + 4;
        vga_draw_string(text_x, text_y, widget->text, widget->text_color);
    }

    // Widget-specific rendering
    switch (widget->type) {
        case WIDGET_BUTTON:
            // Button-specific rendering (already handled by base)
            break;

        case WIDGET_CHECKBOX:
            // Draw checkbox
            uint32_t cb_x = widget->x + 4;
            uint32_t cb_y = widget->y + (widget->height - 12) / 2;
            vga_draw_rect(cb_x, cb_y, 12, 12, widget->border_color);
            if (widget->data.checkbox.checked) {
                vga_fill_rect(cb_x + 2, cb_y + 2, 8, 8, widget->foreground_color);
            }
            // Draw label
            if (widget->data.checkbox.label) {
                vga_draw_string(cb_x + 16, widget->y + (widget->height - 8) / 2,
                               widget->data.checkbox.label, widget->text_color);
            }
            break;

        case WIDGET_PROGRESS_BAR:
            // Draw progress bar background
            vga_draw_rect(widget->x, widget->y, widget->width, widget->height, widget->border_color);
            // Draw progress fill
            uint32_t progress_width = (widget->data.progress_bar.current_value * (widget->width - 4)) /
                                    (widget->data.progress_bar.max_value - widget->data.progress_bar.min_value);
            vga_fill_rect(widget->x + 2, widget->y + 2, progress_width, widget->height - 4, widget->foreground_color);
            break;

        default:
            break;
    }
}

void ui_render_container(ui_container_t* container) {
    if (!container || !container->base.visible) return;

    // Render container background
    ui_render_widget(&container->base);

    // Render children
    for (uint32_t i = 0; i < container->child_count; i++) {
        ui_widget_t* child = container->children[i];
        if (child) {
            if (child->type == WIDGET_WINDOW || child->type == WIDGET_PANEL) {
                ui_render_container((ui_container_t*)child);
            } else {
                ui_render_widget(child);
            }
        }
    }
}

// Event handling (basic implementations)
void ui_process_mouse_event(ui_widget_t* widget, ui_event_t* event) {
    if (!widget || !widget->enabled) return;

    switch (event->type) {
        case EVENT_MOUSE_ENTER:
            widget->hovered = 1;
            if (widget->on_mouse_enter) {
                widget->on_mouse_enter(widget, event);
            }
            break;
        case EVENT_MOUSE_LEAVE:
            widget->hovered = 0;
            if (widget->on_mouse_leave) {
                widget->on_mouse_leave(widget, event);
            }
            break;
        case EVENT_MOUSE_DOWN:
            widget->pressed = 1;
            if (widget->on_mouse_down) {
                widget->on_mouse_down(widget, event);
            }
            break;
        case EVENT_MOUSE_UP:
            widget->pressed = 0;
            if (widget->on_mouse_up) {
                widget->on_mouse_up(widget, event);
            }
            break;
        case EVENT_MOUSE_CLICK:
            if (widget->on_mouse_click) {
                widget->on_mouse_click(widget, event);
            }
            break;
        default:
            break;
    }
}

void ui_process_keyboard_event(ui_widget_t* widget, ui_event_t* event) {
    if (!widget || !widget->enabled || !widget->focused) return;

    switch (event->type) {
        case EVENT_KEY_DOWN:
            if (widget->on_key_down) {
                widget->on_key_down(widget, event);
            }
            break;
        case EVENT_KEY_UP:
            if (widget->on_key_up) {
                widget->on_key_up(widget, event);
            }
            break;
        case EVENT_KEY_PRESS:
            // Handle text input for textboxes
            if (widget->type == WIDGET_TEXTBOX && widget->data.textbox.buffer) {
                // Simple text input (would need more sophisticated handling)
                uint32_t len = strlen(widget->data.textbox.buffer);
                if (len < widget->data.textbox.buffer_size - 1) {
                    widget->data.textbox.buffer[len] = event->key_char;
                    widget->data.textbox.buffer[len + 1] = '\0';
                }
            }
            break;
        default:
            break;
    }
}

void ui_update_widget_state(ui_widget_t* widget, uint32_t mouse_x, uint32_t mouse_y, uint8_t mouse_button_pressed) {
    if (!widget || !widget->enabled) return;

    uint8_t was_hovered = widget->hovered;
    widget->hovered = ui_point_in_widget(widget, mouse_x, mouse_y);

    if (widget->hovered && !was_hovered) {
        ui_event_t event = {EVENT_MOUSE_ENTER, mouse_x, mouse_y, mouse_button_pressed, 0, 0, 0};
        ui_process_mouse_event(widget, &event);
    } else if (!widget->hovered && was_hovered) {
        ui_event_t event = {EVENT_MOUSE_LEAVE, mouse_x, mouse_y, mouse_button_pressed, 0, 0, 0};
        ui_process_mouse_event(widget, &event);
    }

    // Update visual state based on interaction
    if (widget->hovered && mouse_button_pressed) {
        widget->state = STATE_PRESSED;
        widget->background_color = current_theme.button_pressed_bg;
    } else if (widget->hovered) {
        widget->state = STATE_HOVER;
        widget->background_color = current_theme.button_hover_bg;
    } else {
        widget->state = STATE_NORMAL;
        widget->background_color = current_theme.button_normal_bg;
    }
}

// Initialize the UI system
void ui_widgets_init() {
    ui_load_default_theme();
}