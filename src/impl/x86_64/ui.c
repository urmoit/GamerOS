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
    uint32_t offset = 0;
    while (str[offset] != '\0' && offset < 19) {
        if (x + offset * 8 < VGA_WIDTH - 8) {
            draw_char(x + offset * 8, y, str[offset], color);
        }
        offset++;
    }
}

void ui_init(void) {
    // Initialize tabs
    tab_count = 4;
    
    tabs[0].x = 60;
    tabs[0].y = 3;
    tabs[0].width = TAB_WIDTH;
    tabs[0].is_active = 1;
    tabs[0].text[0] = 'H'; tabs[0].text[1] = 'o'; tabs[0].text[2] = 'm'; tabs[0].text[3] = 'e'; tabs[0].text[4] = '\0';
    
    tabs[1].x = 60 + TAB_WIDTH + TAB_SPACING;
    tabs[1].y = 3;
    tabs[1].width = TAB_WIDTH;
    tabs[1].is_active = 0;
    tabs[1].text[0] = 'F'; tabs[1].text[1] = 'i'; tabs[1].text[2] = 'l'; tabs[1].text[3] = 'e'; tabs[1].text[4] = 's'; tabs[1].text[5] = '\0';
    
    tabs[2].x = 60 + (TAB_WIDTH + TAB_SPACING) * 2;
    tabs[2].y = 3;
    tabs[2].width = TAB_WIDTH;
    tabs[2].is_active = 0;
    tabs[2].text[0] = 'S'; tabs[2].text[1] = 'e'; tabs[2].text[2] = 't'; tabs[2].text[3] = 't'; tabs[2].text[4] = 'i'; tabs[2].text[5] = 'n'; tabs[2].text[6] = 'g'; tabs[2].text[7] = 's'; tabs[2].text[8] = '\0';
    
    tabs[3].x = 60 + (TAB_WIDTH + TAB_SPACING) * 3;
    tabs[3].y = 3;
    tabs[3].width = TAB_WIDTH;
    tabs[3].is_active = 0;
    tabs[3].text[0] = 'H'; tabs[3].text[1] = 'e'; tabs[3].text[2] = 'l'; tabs[3].text[3] = 'p'; tabs[3].text[4] = '\0';
}

void ui_draw_tab(uint32_t x, uint32_t y, const char* text, uint8_t is_active) {
    uint8_t bg_color = is_active ? COLOR_TAB_ACTIVE : COLOR_TAB_INACTIVE;
    uint8_t text_color = COLOR_TAB_TEXT;
    
    // Draw tab background
    vga_fill_rect(x, y, TAB_WIDTH, TAB_HEIGHT, bg_color);
    
    // Draw tab border
    vga_draw_rect(x, y, TAB_WIDTH, TAB_HEIGHT, COLOR_BORDER);
    
    // Draw tab text (centered)
    uint32_t text_x = x + (TAB_WIDTH / 2) - 16; // Approximate center
    uint32_t text_y = y + (TAB_HEIGHT / 2) - 4;
    draw_string(text_x, text_y, text, text_color);
}

void ui_draw_header(void) {
    // Draw header background
    vga_fill_rect(0, 0, VGA_WIDTH, HEADER_HEIGHT, COLOR_HEADER_BG);
    
    // Draw header border
    vga_draw_rect(0, 0, VGA_WIDTH, HEADER_HEIGHT, COLOR_BORDER);
    
    // Draw bottom border of header (separator)
    for (uint32_t x = 0; x < VGA_WIDTH; x++) {
        vga_set_pixel(x, HEADER_HEIGHT - 1, COLOR_BORDER);
    }

    // Draw Start button
    vga_fill_rect(5, 5, 50, 20, COLOR_LIGHT_GREEN);
    draw_string(10, 10, "Start", COLOR_BLACK);
    
    // Draw all tabs
    for (uint8_t i = 0; i < tab_count; i++) {
        ui_draw_tab(tabs[i].x, tabs[i].y, tabs[i].text, tabs[i].is_active);
    }
    
    // Draw title on the right side
    uint32_t title_x = VGA_WIDTH - 80;
    draw_string(title_x, 10, "GamerOS", COLOR_WHITE);
}

void ui_draw_setup_screen(void) {
    // Clear screen with a setup background color
    vga_clear(COLOR_SETUP_BG);

    // Draw a title
    draw_string(100, 50, "Welcome to GamerOS", COLOR_WHITE);

    // Draw a button
    vga_fill_rect(110, 100, 100, 30, COLOR_LIGHT_BLUE);
    vga_draw_rect(110, 100, 100, 30, COLOR_WHITE);
    draw_string(120, 110, "Start Setup", COLOR_BLACK);
}

void ui_handle_setup(void) {
    // For now, just wait for a key press to continue
    // In a real scenario, this would handle mouse clicks or keyboard input
    // to navigate the setup process.
    // Since we don't have a proper input system yet, we'll just hang here.
    // The user would need to reboot to get out of this state.

    // Fixed: Add a timeout or exit condition to prevent infinite loop
    // For now, just return after a short delay to allow the OS to continue
    for(int i = 0; i < 1000000; i++) {
        __asm__("nop"); // Small delay
    }
    // Return to allow OS to continue booting
}

static int is_start_menu_open = 0;

void ui_draw_start_menu(void) {
    if (is_start_menu_open) {
        vga_fill_rect(0, HEADER_HEIGHT, START_MENU_WIDTH, START_MENU_HEIGHT, COLOR_START_MENU_BG);
        vga_draw_rect(0, HEADER_HEIGHT, START_MENU_WIDTH, START_MENU_HEIGHT, COLOR_WHITE);
        draw_string(10, HEADER_HEIGHT + 10, "Start Menu", COLOR_WHITE);
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

    draw_string(VGA_WIDTH - 80, VGA_HEIGHT - 20, time_str, COLOR_WHITE);
}

void ui_draw_window_list() {
    int x = 60;
    for (int i = 0; i < window_count; i++) {
        if (windows[i]) {
            draw_string(x, VGA_HEIGHT - 20, windows[i]->title, COLOR_WHITE);
            x += 80;
        }
    }
}

void ui_draw_taskbar(void) {
    vga_fill_rect(0, VGA_HEIGHT - TASKBAR_HEIGHT, VGA_WIDTH, TASKBAR_HEIGHT, COLOR_TASKBAR_BG);
    vga_draw_rect(0, VGA_HEIGHT - TASKBAR_HEIGHT, VGA_WIDTH, TASKBAR_HEIGHT, COLOR_WHITE);
    ui_draw_clock();
    ui_draw_window_list();
}

