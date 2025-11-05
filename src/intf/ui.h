#ifndef UI_H
#define UI_H

#include "stdint.h"

// Color definitions for VGA mode 13h
#define COLOR_BLACK 0
#define COLOR_BLUE 1
#define COLOR_GREEN 2
#define COLOR_CYAN 3
#define COLOR_RED 4
#define COLOR_MAGENTA 5
#define COLOR_BROWN 6
#define COLOR_LIGHT_GREY 7
#define COLOR_DARK_GREY 8
#define COLOR_LIGHT_BLUE 9
#define COLOR_LIGHT_GREEN 10
#define COLOR_LIGHT_CYAN 11
#define COLOR_LIGHT_RED 12
#define COLOR_LIGHT_MAGENTA 13
#define COLOR_YELLOW 14
#define COLOR_WHITE 15

// Custom UI colors
#define COLOR_HEADER_BG 8      // Dark grey
#define COLOR_TAB_ACTIVE 9     // Light blue
#define COLOR_TAB_INACTIVE 7   // Light grey
#define COLOR_TAB_TEXT 0        // Black
#define COLOR_BORDER 15         // White
#define COLOR_SETUP_BG 2        // Green

// Header dimensions
#define HEADER_HEIGHT 30
#define TAB_WIDTH 100
#define TAB_HEIGHT 25
#define TAB_SPACING 2

// Tab structure
typedef struct {
    uint32_t x;
    uint32_t y;
    uint32_t width;
    char text[20];
    uint8_t is_active;
} tab_t;

// Initialize UI system
void ui_init(void);

// Draw header with tabs
void ui_draw_header(void);

// Draw a tab button
void ui_draw_tab(uint32_t x, uint32_t y, const char* text, uint8_t is_active);

// Draw setup screen
void ui_draw_setup_screen(void);

// Handle setup input
void ui_handle_setup(void);

// Start Menu
#define START_MENU_WIDTH 120
#define START_MENU_HEIGHT 150
#define COLOR_START_MENU_BG COLOR_DARK_GREY

void ui_draw_start_menu(void);
void ui_toggle_start_menu(void);

// Taskbar
#define TASKBAR_HEIGHT 30
#define COLOR_TASKBAR_BG COLOR_DARK_GREY

void ui_draw_taskbar(void);
void ui_draw_clock(void);

void draw_string(uint32_t x, uint32_t y, const char* str, uint8_t color);

#endif



