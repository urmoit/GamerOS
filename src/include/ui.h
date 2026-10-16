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

#define COLOR_HEADER_BG 8
#define COLOR_TAB_ACTIVE 9
#define COLOR_TAB_INACTIVE 7
#define COLOR_TAB_TEXT 0
#define COLOR_BORDER 15
#define COLOR_SETUP_BG 2

#define HEADER_HEIGHT 30
#define TAB_WIDTH 100
#define TAB_HEIGHT 25
#define TAB_SPACING 2

typedef struct {
    uint32_t x;
    uint32_t y;
    uint32_t width;
    char text[20];
    uint8_t is_active;
} tab_t;

void ui_init(void);
void ui_draw_header(void);
void ui_draw_tab(uint32_t x, uint32_t y, const char* text, uint8_t is_active);
void ui_draw_setup_screen(void);
void ui_handle_setup(void);

#define START_MENU_WIDTH 120
#define START_MENU_HEIGHT 150
#define COLOR_START_MENU_BG COLOR_DARK_GREY

void ui_draw_start_menu(void);
void ui_toggle_start_menu(void);
int ui_is_start_menu_open(void);
void ui_draw_desktop_background(void);
void ui_draw_desktop_icons(void);

#define TASKBAR_HEIGHT 30
#define COLOR_TASKBAR_BG COLOR_DARK_GREY

void ui_draw_taskbar(void);
void ui_draw_clock(void);

// draw_string is declared in graphics.c with int coordinates; do not redeclare.

// TODO: Add support for multiple UI themes and skins
// TODO: Implement internationalization (i18n) for UI strings
// TODO: Add accessibility features (high contrast, large fonts)
// TODO: Implement UI animation and transition effects

#endif



