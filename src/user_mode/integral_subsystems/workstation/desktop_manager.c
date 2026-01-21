#include "desktop_manager.h"
#include "../../../intf/graphics.h"
#include "../../../intf/ui.h"
#include "../../../intf/ports.h"

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
    // Draw four colored bars directly to framebuffer to test palette initialization
    // Use volatile pointer to ensure writes are not optimized away
    volatile uint8_t* fb = (volatile uint8_t*)0xA0000;
    
    // White bar (top 50 rows)
    for (uint32_t y = 0; y < 50; y++) {
        for (uint32_t x = 0; x < 320; x++) {
            fb[y * 320 + x] = 0x0F; // White (palette 15)
        }
    }
    
    // Red bar (next 50 rows)
    for (uint32_t y = 50; y < 100; y++) {
        for (uint32_t x = 0; x < 320; x++) {
            fb[y * 320 + x] = 0x04; // Red (palette 4)
        }
    }
    
    // Green bar (next 50 rows)
    for (uint32_t y = 100; y < 150; y++) {
        for (uint32_t x = 0; x < 320; x++) {
            fb[y * 320 + x] = 0x02; // Green (palette 2)
        }
    }
    
    // Blue bar (bottom 50 rows)
    for (uint32_t y = 150; y < 200; y++) {
        for (uint32_t x = 0; x < 320; x++) {
            fb[y * 320 + x] = 0x01; // Blue (palette 1)
        }
    }
    
    // Memory barrier to ensure all writes complete
    __asm__ volatile("mfence" ::: "memory");

    // Draw initial desktop elements
    if (desktop_config.show_taskbar) {
        desktop_show_taskbar();
    }

    // TODO: Draw desktop icons
    // TODO: Initialize desktop widgets
}

void desktop_set_wallpaper(const char* wallpaper_path) {
    // TODO: Load and set wallpaper
    // For now, just set a solid color background
    vga_clear(COLOR_BLUE);
}

void desktop_add_icon(const char* icon_path, int x, int y, const char* label) {
    // TODO: Load icon and add to desktop
    // For now, draw a simple placeholder
    vga_fill_rect(x, y, 32, 32, COLOR_LIGHT_BLUE);
    draw_string(x, y + 35, label, COLOR_WHITE);
}

void desktop_show_taskbar(void) {
    ui_draw_taskbar();
}

void desktop_hide_taskbar(void) {
    // TODO: Hide taskbar
}

void desktop_toggle_start_menu(void) {
    start_menu_open = !start_menu_open;
    ui_toggle_start_menu();
}

void desktop_update_clock(void) {
    ui_draw_clock();
}