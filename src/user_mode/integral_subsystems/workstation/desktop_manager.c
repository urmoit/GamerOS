#include "desktop_manager.h"
#include "../../../intf/graphics.h"
#include "../../../intf/ui.h"
#include "../../../intf/ports.h"
#include "../../../intf/mouse.h"

// Desktop state
static desktop_config_t desktop_config;
static uint8_t start_menu_open = 0;

// Desktop manager functions
void desktop_manager_init(void) {
    // Initialize desktop configuration
    desktop_config.width = 320;
    desktop_config.height = 200;
    desktop_config.color_depth = 256;
    desktop_config.show_taskbar = 1;
    desktop_config.show_start_menu = 0;
    desktop_config.show_clock = 1;

    start_menu_open = 0;
}

void desktop_manager_shutdown(void) {
    // Cleanup desktop resources
}

void desktop_create(void) {
    // Draw XP-style desktop background
    ui_draw_desktop_background();
    
    // Draw desktop icons
    ui_draw_desktop_icons();

    // Draw taskbar
    if (desktop_config.show_taskbar) {
        desktop_show_taskbar();
    }
    
    // Memory barrier to ensure all writes complete
    __asm__ volatile("mfence" ::: "memory");
}

void desktop_set_wallpaper(const char* wallpaper_path) {
    // For now, just set the XP desktop background
    (void)wallpaper_path;
    ui_draw_desktop_background();
}

void desktop_add_icon(const char* icon_path, int x, int y, const char* label) {
    // TODO: Load icon and add to desktop
    (void)icon_path;
    (void)x;
    (void)y;
    (void)label;
}

void desktop_show_taskbar(void) {
    ui_draw_taskbar();
}

void desktop_hide_taskbar(void) {
    // Redraw desktop to hide taskbar
    ui_draw_desktop_background();
    ui_draw_desktop_icons();
}

void desktop_toggle_start_menu(void) {
    start_menu_open = !start_menu_open;
    ui_toggle_start_menu();
}

void desktop_update_clock(void) {
    ui_draw_clock();
}

// Handle mouse click on desktop
void desktop_handle_mouse_click(int32_t x, int32_t y, uint8_t buttons) {
    (void)buttons;
    
    // Check if Start button clicked (bottom left area)
    if (y >= 180 && y <= 200 && x >= 5 && x <= 55) {
        desktop_toggle_start_menu();
        return;
    }
    
    // If clicking outside start menu, close it
    if (start_menu_open) {
        if (x >= (int32_t)START_MENU_WIDTH || y >= (int32_t)(HEADER_HEIGHT + START_MENU_HEIGHT)) {
            desktop_toggle_start_menu();
        }
    }
}

// Main desktop update loop
void desktop_update(void) {
    // Get mouse state
    mouse_state_t mouse = mouse_get_state();
    
    // Handle mouse clicks
    static uint8_t last_buttons = 0;
    if (mouse.buttons && !last_buttons) {
        // Mouse button just pressed
        desktop_handle_mouse_click(mouse.x, mouse.y, mouse.buttons);
    }
    last_buttons = mouse.buttons;
    
    // Redraw desktop elements
    desktop_create();
    
    // Draw start menu if open
    if (ui_is_start_menu_open()) {
        ui_draw_start_menu();
    }
}
