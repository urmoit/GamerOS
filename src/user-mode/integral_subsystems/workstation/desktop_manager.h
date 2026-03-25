#ifndef DESKTOP_MANAGER_H
#define DESKTOP_MANAGER_H

#include <stdint.h>

// Desktop configuration
typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t color_depth;
    uint8_t show_taskbar;
    uint8_t show_start_menu;
    uint8_t show_clock;
} desktop_config_t;

// Desktop manager functions
void desktop_manager_init(void);
void desktop_manager_shutdown(void);
void desktop_create(void);
void desktop_set_wallpaper(const char* wallpaper_path);
void desktop_add_icon(const char* icon_path, int x, int y, const char* label);
void desktop_show_taskbar(void);
void desktop_hide_taskbar(void);
void desktop_toggle_start_menu(void);
void desktop_update_clock(void);
void desktop_handle_mouse_click(int32_t x, int32_t y, uint8_t buttons);
void desktop_update(void);

#endif