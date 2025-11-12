#include "../../intf/ui.h"
#include "../../intf/graphics.h"
#include "../../intf/window.h"
#include "../../intf/rtc.h"

#define MAX_TABS 5

static tab_t tabs[MAX_TABS];
static uint8_t tab_count = 0;

// Simple 8x8 pixel font drawing
static void draw_char(uint32_t x, uint32_t y, char c, uint8_t color) {
    if (c == ' ') return; // Skip spaces
    
    // Very basic character rendering using rectangles
    // This is a minimal implementation - for production, use a proper bitmap font
    uint8_t char_width = 6;
    uint8_t char_height = 8;
    
    // Draw characters as simple patterns
    switch (c) {
        case 'H': case 'h':
            vga_fill_rect(x + 1, y, 1, char_height, color);
            vga_fill_rect(x + 4, y, 1, char_height, color);
            vga_fill_rect(x + 1, y + 3, 4, 1, color);
            break;
        case 'O': case 'o':
            vga_draw_rect(x, y + 1, char_width, char_height - 2, color);
            break;
        case 'M': case 'm':
            vga_fill_rect(x, y, 1, char_height, color);
            vga_fill_rect(x + 5, y, 1, char_height, color);
            vga_fill_rect(x + 2, y + 1, 1, 2, color);
            break;
        case 'E': case 'e':
            vga_fill_rect(x, y, 1, char_height, color);
            vga_fill_rect(x, y, char_width, 1, color);
            vga_fill_rect(x, y + 3, char_width - 1, 1, color);
            vga_fill_rect(x, y + char_height - 1, char_width, 1, color);
            break;
        case 'F': case 'f':
            vga_fill_rect(x, y, 1, char_height, color);
            vga_fill_rect(x, y, char_width, 1, color);
            vga_fill_rect(x, y + 3, char_width - 1, 1, color);
            break;
        case 'I': case 'i':
            vga_fill_rect(x + 2, y, 1, char_height, color);
            break;
        case 'L': case 'l':
            vga_fill_rect(x, y, 1, char_height, color);
            vga_fill_rect(x, y + char_height - 1, char_width, 1, color);
            break;
        case 'S': case 's':
            vga_fill_rect(x, y, char_width, 1, color);
            vga_fill_rect(x, y + 3, char_width, 1, color);
            vga_fill_rect(x, y + char_height - 1, char_width, 1, color);
            vga_fill_rect(x, y, 1, 4, color);
            vga_fill_rect(x + char_width - 1, y + 4, 1, 4, color);
            break;
        case 'T': case 't':
            vga_fill_rect(x, y, char_width, 1, color);
            vga_fill_rect(x + 2, y, 1, char_height, color);
            break;
        case 'N': case 'n':
            vga_fill_rect(x, y, 1, char_height, color);
            vga_fill_rect(x + 5, y, 1, char_height, color);
            for (uint8_t i = 1; i < 5; i++) {
                vga_fill_rect(x + i, y + i, 1, 1, color);
            }
            break;
        case 'G': case 'g':
            vga_draw_rect(x, y + 1, char_width, char_height - 2, color);
            vga_fill_rect(x + 4, y + 3, 2, 2, color);
            break;
        case 'A': case 'a':
            vga_fill_rect(x, y + char_height - 1, 1, 1, color);
            vga_fill_rect(x + 5, y + char_height - 1, 1, 1, color);
            vga_fill_rect(x + 1, y, 1, char_height, color);
            vga_fill_rect(x + 4, y, 1, char_height, color);
            vga_fill_rect(x + 1, y + 3, 4, 1, color);
            break;
        case 'R': case 'r':
            vga_fill_rect(x, y, 1, char_height, color);
            vga_fill_rect(x, y, char_width, 1, color);
            vga_fill_rect(x, y + 3, char_width - 1, 1, color);
            vga_fill_rect(x + 4, y + 1, 1, 2, color);
            vga_fill_rect(x + 3, y + 4, 1, 4, color);
            break;
        default:
            // Draw a simple rectangle for unknown characters
            vga_fill_rect(x + 1, y + 2, 4, 4, color);
            break;
    }
}

void draw_string(uint32_t x, uint32_t y, const char* str, uint8_t color) {
    if (!str) return; // NULL check
    uint32_t offset = 0;
    uint32_t max_chars = 19; // Maximum characters to draw
    uint32_t char_width = 8; // Character width
    while (str[offset] != '\0' && offset < max_chars) {
        if (x + offset * char_width < 320 - char_width) {
            draw_char(x + offset * char_width, y, str[offset], color);
        }
        offset++;
    }
}

// Define constants for magic numbers
#define VGA_WIDTH_CONSTANT  320
#define VGA_HEIGHT_CONSTANT 200
#define HEADER_HEIGHT_CONSTANT 30
#define TAB_WIDTH_CONSTANT 80
#define TAB_HEIGHT_CONSTANT 25
#define TAB_SPACING_CONSTANT 5
#define START_MENU_WIDTH_CONSTANT 200
#define START_MENU_HEIGHT_CONSTANT 150
#define TASKBAR_HEIGHT_CONSTANT 25
#define COLOR_HEADER_BG_CONSTANT 0x1F
#define COLOR_TAB_ACTIVE_CONSTANT 0x2F
#define COLOR_TAB_INACTIVE_CONSTANT 0x17
#define COLOR_TAB_TEXT_CONSTANT 0x3F
#define COLOR_BORDER_CONSTANT 0x0F
#define COLOR_LIGHT_GREEN_CONSTANT 0x2A
#define COLOR_LIGHT_BLUE_CONSTANT 0x39
#define COLOR_SETUP_BG_CONSTANT 0x10
#define COLOR_WHITE_CONSTANT 0x3F
#define COLOR_BLACK_CONSTANT 0x00
#define COLOR_DARK_GREY_CONSTANT 0x07
#define COLOR_BLUE_CONSTANT 0x01
#define COLOR_TASKBAR_BG_CONSTANT 0x07
#define COLOR_START_MENU_BG_CONSTANT 0x1F

void ui_init(void) {
    // Initialize tabs
    tab_count = 4;

    tabs[0].x = 60;
    tabs[0].y = 3;
    tabs[0].width = TAB_WIDTH_CONSTANT;
    tabs[0].is_active = 1;
    tabs[0].text[0] = 'H'; tabs[0].text[1] = 'o'; tabs[0].text[2] = 'm'; tabs[0].text[3] = 'e'; tabs[0].text[4] = '\0';

    tabs[1].x = 60 + TAB_WIDTH_CONSTANT + TAB_SPACING_CONSTANT;
    tabs[1].y = 3;
    tabs[1].width = TAB_WIDTH_CONSTANT;
    tabs[1].is_active = 0;
    tabs[1].text[0] = 'F'; tabs[1].text[1] = 'i'; tabs[1].text[2] = 'l'; tabs[1].text[3] = 'e'; tabs[1].text[4] = 's'; tabs[1].text[5] = '\0';

    tabs[2].x = 60 + (TAB_WIDTH_CONSTANT + TAB_SPACING_CONSTANT) * 2;
    tabs[2].y = 3;
    tabs[2].width = TAB_WIDTH_CONSTANT;
    tabs[2].is_active = 0;
    tabs[2].text[0] = 'S'; tabs[2].text[1] = 'e'; tabs[2].text[2] = 't'; tabs[2].text[3] = 't'; tabs[2].text[4] = 'i'; tabs[2].text[5] = 'n'; tabs[2].text[6] = 'g'; tabs[2].text[7] = 's'; tabs[2].text[8] = '\0';

    tabs[3].x = 60 + (TAB_WIDTH_CONSTANT + TAB_SPACING_CONSTANT) * 3;
    tabs[3].y = 3;
    tabs[3].width = TAB_WIDTH_CONSTANT;
    tabs[3].is_active = 0;
    tabs[3].text[0] = 'H'; tabs[3].text[1] = 'e'; tabs[3].text[2] = 'l'; tabs[3].text[3] = 'p'; tabs[3].text[4] = '\0';
}

void ui_draw_tab(uint32_t x, uint32_t y, const char* text, uint8_t is_active) {
    uint8_t bg_color = is_active ? COLOR_TAB_ACTIVE_CONSTANT : COLOR_TAB_INACTIVE_CONSTANT;
    uint8_t text_color = COLOR_TAB_TEXT_CONSTANT;

    // Draw tab background
    vga_fill_rect(x, y, TAB_WIDTH_CONSTANT, TAB_HEIGHT_CONSTANT, bg_color);

    // Draw tab border
    vga_draw_rect(x, y, TAB_WIDTH_CONSTANT, TAB_HEIGHT_CONSTANT, COLOR_BORDER_CONSTANT);

    // Draw tab text (centered)
    uint32_t text_x = x + (TAB_WIDTH_CONSTANT / 2) - 16; // Approximate center
    uint32_t text_y = y + (TAB_HEIGHT_CONSTANT / 2) - 4;
    draw_string(text_x, text_y, text, text_color);
}

void ui_draw_header(void) {
    // Draw header background
    vga_fill_rect(0, 0, VGA_WIDTH_CONSTANT, HEADER_HEIGHT_CONSTANT, COLOR_HEADER_BG_CONSTANT);

    // Draw header border
    vga_draw_rect(0, 0, VGA_WIDTH_CONSTANT, HEADER_HEIGHT_CONSTANT, COLOR_BORDER_CONSTANT);

    // Draw bottom border of header (separator)
    for (size_t x = 0; x < VGA_WIDTH_CONSTANT; x++) {
        vga_set_pixel((uint32_t)x, HEADER_HEIGHT_CONSTANT - 1, COLOR_BORDER_CONSTANT);
    }

    // Draw Start button
    vga_fill_rect(5, 5, 50, 20, COLOR_LIGHT_GREEN_CONSTANT);
    draw_string(10, 10, "Start", COLOR_BLACK_CONSTANT);

    // Draw all tabs
    for (size_t i = 0; i < tab_count; i++) {
        ui_draw_tab(tabs[i].x, tabs[i].y, tabs[i].text, tabs[i].is_active);
    }

    // Draw title on the right side
    uint32_t title_x = VGA_WIDTH_CONSTANT - 80;
    draw_string(title_x, 10, "GamerOS", COLOR_WHITE_CONSTANT);
}

void ui_draw_setup_screen(void) {
    // Clear screen with a setup background color
    vga_clear(COLOR_SETUP_BG_CONSTANT);

    // Draw a title
    draw_string(100, 50, "Welcome to GamerOS", COLOR_WHITE_CONSTANT);

    // Draw a button
    vga_fill_rect(110, 100, 100, 30, COLOR_LIGHT_BLUE_CONSTANT);
    vga_draw_rect(110, 100, 100, 30, COLOR_WHITE_CONSTANT);
    draw_string(120, 110, "Start Setup", COLOR_BLACK_CONSTANT);
}

void ui_handle_setup(void) {
    // For now, just wait for a key press to continue
    // In a real scenario, this would handle mouse clicks or keyboard input
    // to navigate the setup process.
    // Since we don't have a proper input system yet, we'll just hang here.
    // The user would need to reboot to get out of this state.

    // Fixed: Add a timeout or exit condition to prevent infinite loop
    // For now, just return after a short delay to allow the OS to continue
    for(size_t i = 0; i < 1000000; i++) {
        __asm__("nop"); // Small delay
    }
    // Return to allow OS to continue booting
}

static int is_start_menu_open = 0;

void ui_draw_start_menu(void) {
    if (is_start_menu_open) {
        vga_fill_rect(0, HEADER_HEIGHT_CONSTANT, START_MENU_WIDTH_CONSTANT, START_MENU_HEIGHT_CONSTANT, COLOR_START_MENU_BG_CONSTANT);
        vga_draw_rect(0, HEADER_HEIGHT_CONSTANT, START_MENU_WIDTH_CONSTANT, START_MENU_HEIGHT_CONSTANT, COLOR_WHITE_CONSTANT);
        draw_string(10, HEADER_HEIGHT_CONSTANT + 10, "Start Menu", COLOR_WHITE_CONSTANT);
    }
}

void ui_toggle_start_menu(void) {
    is_start_menu_open = !is_start_menu_open;
}

void ui_draw_clock(void) {
    uint8_t hour, minute, second;
    get_time(&hour, &minute, &second);

    // Handle 12/24 hour format - assume 24-hour for now
    // In a real OS, this would be configurable
    char time_str[9];
    time_str[0] = (hour / 10) + '0';
    time_str[1] = (hour % 10) + '0';
    time_str[2] = ':';
    time_str[3] = (minute / 10) + '0';
    time_str[4] = (minute % 10) + '0';
    time_str[5] = ':';
    time_str[6] = (second / 10) + '0';
    time_str[7] = (second % 10) + '0';
    time_str[8] = '\0';

    uint32_t clock_x = VGA_WIDTH_CONSTANT - 80; // Clock X position
    uint32_t clock_y = VGA_HEIGHT_CONSTANT - 20; // Clock Y position
    draw_string(clock_x, clock_y, time_str, COLOR_WHITE_CONSTANT);
}

void ui_draw_window_list() {
    int start_x = 60; // Starting X position for window list
    int window_spacing = 80; // Spacing between window titles
    int taskbar_y = VGA_HEIGHT_CONSTANT - 20; // Taskbar text Y position
    int current_x = start_x;
    for (size_t i = 0; i < window_count; i++) {
        if (windows[i] && windows[i]->title) {
            draw_string((uint32_t)current_x, (uint32_t)taskbar_y, windows[i]->title, COLOR_WHITE_CONSTANT);
            current_x += window_spacing;
        }
    }
}

void ui_draw_taskbar(void) {
    vga_fill_rect(0, VGA_HEIGHT_CONSTANT - TASKBAR_HEIGHT_CONSTANT, VGA_WIDTH_CONSTANT, TASKBAR_HEIGHT_CONSTANT, COLOR_TASKBAR_BG_CONSTANT);
    vga_draw_rect(0, VGA_HEIGHT_CONSTANT - TASKBAR_HEIGHT_CONSTANT, VGA_WIDTH_CONSTANT, TASKBAR_HEIGHT_CONSTANT, COLOR_WHITE_CONSTANT);
    ui_draw_clock();
    ui_draw_window_list();
}

