#include "../../intf/ui.h"
#include "../../intf/graphics.h"
#include "../../intf/window.h"
#include "../../intf/rtc.h"
#include "../../intf/mouse.h"

#define MAX_TABS 5

static tab_t tabs[MAX_TABS];
static uint8_t tab_count = 0;

// Windows XP Luna colors (using VGA palette indices that approximate XP colors)
#define XP_BLACK        0x00  // Black
#define XP_DARK_BLUE    0x01  // Dark Blue (for title bars)
#define XP_GREEN        0x02  // Green (for start button)
#define XP_TEAL         0x03  // Teal
#define XP_DARK_RED     0x04  // Dark Red
#define XP_PURPLE       0x05  // Purple
#define XP_BROWN        0x06  // Brown/Olive
#define XP_LIGHT_GRAY   0x07  // Light Gray
#define XP_DARK_GRAY    0x08  // Dark Gray
#define XP_LIGHT_BLUE   0x09  // Light Blue
#define XP_LIGHT_GREEN  0x0A  // Light Green
#define XP_LIGHT_CYAN   0x0B  // Light Cyan
#define XP_LIGHT_RED    0x0C  // Light Red
#define XP_LIGHT_PURPLE 0x0D  // Light Purple
#define XP_YELLOW       0x0E  // Yellow
#define XP_WHITE        0x0F  // White

// XP-style color scheme for VGA mode 13h
#define XP_DESKTOP_BG       0x39  // Light blue/teal (simulating XP bliss wallpaper)
#define XP_TASKBAR_BG       0x01  // Blue
#define XP_TASKBAR_HIGHLIGHT 0x09 // Light blue
#define XP_START_BTN_GREEN  0x02  // Green
#define XP_START_BTN_LIGHT  0x0A  // Light green
#define XP_WINDOW_BG        0x0F  // White
#define XP_WINDOW_TITLE     0x01  // Blue
#define XP_TEXT_DARK        0x00  // Black
#define XP_TEXT_LIGHT       0x0F  // White
#define XP_BORDER_DARK      0x08  // Dark gray
#define XP_BORDER_LIGHT     0x07  // Light gray

// Simple 8x8 pixel font drawing using the proper font from font.h
extern const uint8_t font_8x8[96][8];

static void draw_char(uint32_t x, uint32_t y, char c, uint8_t color) {
    if ((uint8_t)c < 32 || (uint8_t)c > 126) return;
    
    const uint8_t* char_bitmap = font_8x8[(uint8_t)c - 32];
    
    for (uint32_t row = 0; row < 8; row++) {
        uint8_t row_data = char_bitmap[row];
        for (uint32_t col = 0; col < 8; col++) {
            if (row_data & (1 << (7 - col))) {
                if (x + col < 320 && y + row < 200) {
                    vga_set_pixel(x + col, y + row, color);
                }
            }
        }
    }
}

void draw_string(uint32_t x, uint32_t y, const char* str, uint8_t color) {
    if (!str) return;
    uint32_t current_x = x;
    while (*str) {
        if (*str == '\n') {
            y += 10;
            current_x = x;
        } else {
            if (current_x < 320 - 8 && y < 200 - 8) {
                draw_char(current_x, y, *str, color);
            }
            current_x += 8;
        }
        str++;
    }
}

// Draw XP-style gradient (simulated with horizontal lines)
static void draw_xp_gradient(uint32_t x, uint32_t y, uint32_t width, uint32_t height, 
                              uint8_t color_top, uint8_t color_bottom) {
    for (uint32_t row = 0; row < height && (y + row) < 200; row++) {
        // Simple gradient by alternating colors
        uint8_t color = (row < height / 2) ? color_top : color_bottom;
        for (uint32_t col = 0; col < width && (x + col) < 320; col++) {
            vga_set_pixel(x + col, y + row, color);
        }
    }
}

// Draw XP-style button
static void draw_xp_button(uint32_t x, uint32_t y, uint32_t width, uint32_t height, 
                            const char* text, uint8_t is_pressed) {
    // Button background with gradient effect
    draw_xp_gradient(x, y, width, height, XP_LIGHT_BLUE, XP_TASKBAR_BG);
    
    // Button border (3D effect)
    // Top and left - highlight
    for (uint32_t i = 0; i < width && (x + i) < 320; i++) {
        vga_set_pixel(x + i, y, XP_WHITE);
    }
    for (uint32_t i = 0; i < height && (y + i) < 200; i++) {
        vga_set_pixel(x, y + i, XP_WHITE);
    }
    
    // Bottom and right - shadow
    for (uint32_t i = 0; i < width && (x + i) < 320; i++) {
        vga_set_pixel(x + i, y + height - 1, XP_BLACK);
    }
    for (uint32_t i = 0; i < height && (y + i) < 200; i++) {
        vga_set_pixel(x + width - 1, y + i, XP_BLACK);
    }
    
    // Button text (centered)
    if (text) {
        uint32_t text_len = 0;
        const char* tmp = text;
        while (*tmp++) text_len++;
        uint32_t text_x = x + (width - text_len * 8) / 2;
        uint32_t text_y = y + (height - 8) / 2;
        draw_string(text_x, text_y, text, XP_TEXT_DARK);
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
    uint8_t bg_color = is_active ? XP_LIGHT_BLUE : XP_DARK_GRAY;
    uint8_t text_color = is_active ? XP_BLACK : XP_WHITE;

    // Draw tab background
    vga_fill_rect(x, y, TAB_WIDTH, TAB_HEIGHT, bg_color);

    // Draw tab border
    vga_draw_rect(x, y, TAB_WIDTH, TAB_HEIGHT, XP_WHITE);

    // Draw tab text (centered)
    uint32_t text_len = 0;
    const char* tmp = text;
    while (*tmp++) text_len++;
    uint32_t text_x = x + (TAB_WIDTH - text_len * 8) / 2;
    uint32_t text_y = y + (TAB_HEIGHT - 8) / 2;
    draw_string(text_x, text_y, text, text_color);
}

void ui_draw_header(void) {
    // Draw header background (XP blue)
    vga_fill_rect(0, 0, 320, HEADER_HEIGHT, XP_TASKBAR_BG);

    // Draw header border
    vga_draw_rect(0, 0, 320, HEADER_HEIGHT, XP_WHITE);

    // Draw Start button (XP green style)
    draw_xp_button(5, 5, 50, 20, "Start", 0);

    // Draw all tabs
    for (size_t i = 0; i < tab_count; i++) {
        ui_draw_tab(tabs[i].x, tabs[i].y, tabs[i].text, tabs[i].is_active);
    }

    // Draw title on the right side
    draw_string(240, 10, "GamerOS", XP_WHITE);
}

void ui_draw_setup_screen(void) {
    // Clear screen with XP desktop color
    vga_clear(XP_DESKTOP_BG);

    // Draw a title
    draw_string(80, 50, "Welcome to GamerOS", XP_WHITE);

    // Draw a button
    draw_xp_button(110, 100, 100, 30, "Start Setup", 0);
}

void ui_handle_setup(void) {
    // Add a timeout or exit condition to prevent infinite loop
    for(size_t i = 0; i < 1000000; i++) {
        __asm__("nop");
    }
}

static int is_start_menu_open = 0;

void ui_draw_start_menu(void) {
    if (is_start_menu_open) {
        // Start menu background
        vga_fill_rect(0, HEADER_HEIGHT, START_MENU_WIDTH, START_MENU_HEIGHT, XP_WINDOW_BG);
        vga_draw_rect(0, HEADER_HEIGHT, START_MENU_WIDTH, START_MENU_HEIGHT, XP_BORDER_DARK);
        
        // Start menu header (blue)
        vga_fill_rect(1, HEADER_HEIGHT + 1, START_MENU_WIDTH - 2, 25, XP_TASKBAR_BG);
        draw_string(10, HEADER_HEIGHT + 8, "GamerOS", XP_WHITE);
        
        // Menu items
        const char* menu_items[] = {
            "Programs",
            "Documents",
            "Settings",
            "Search",
            "Help",
            "Run...",
            "Shut Down"
        };
        
        for (int i = 0; i < 7; i++) {
            uint32_t item_y = HEADER_HEIGHT + 30 + i * 16;
            // Draw item background (highlight if hovered)
            vga_fill_rect(2, item_y, START_MENU_WIDTH - 4, 15, XP_WINDOW_BG);
            draw_string(10, item_y + 4, (char*)menu_items[i], XP_TEXT_DARK);
        }
        
        // Separator line
        for (uint32_t x = 5; x < START_MENU_WIDTH - 5; x++) {
            vga_set_pixel(x, HEADER_HEIGHT + 138, XP_BORDER_DARK);
        }
    }
}

void ui_toggle_start_menu(void) {
    is_start_menu_open = !is_start_menu_open;
}

int ui_is_start_menu_open(void) {
    return is_start_menu_open;
}

void ui_draw_clock(void) {
    uint8_t hour, minute, second;
    get_time(&hour, &minute, &second);

    // Format time string
    char time_str[9];
    time_str[0] = (hour / 10) + '0';
    time_str[1] = (hour % 10) + '0';
    time_str[2] = ':';
    time_str[3] = (minute / 10) + '0';
    time_str[4] = (minute % 10) + '0';
    time_str[5] = '\0';
    
    // Draw clock background
    vga_fill_rect(270, 185, 45, 12, XP_TASKBAR_BG);
    vga_draw_rect(270, 185, 45, 12, XP_BORDER_LIGHT);
    
    // Draw time
    draw_string(275, 187, time_str, XP_WHITE);
}

void ui_draw_window_list() {
    int start_x = 60;
    int window_spacing = 80;
    int taskbar_y = 200 - 20;
    int current_x = start_x;
    
    for (size_t i = 0; i < (size_t)window_count && i < 3; i++) {
        if (windows[i] && windows[i]->title) {
            // Draw window button on taskbar
            draw_xp_button(current_x, taskbar_y - 5, 70, 18, windows[i]->title, !windows[i]->is_active);
            current_x += window_spacing;
        }
    }
}

void ui_draw_taskbar(void) {
    // Draw taskbar background (XP blue with gradient)
    draw_xp_gradient(0, 200 - TASKBAR_HEIGHT, 320, TASKBAR_HEIGHT, XP_TASKBAR_HIGHLIGHT, XP_TASKBAR_BG);
    
    // Draw taskbar border
    for (uint32_t x = 0; x < 320; x++) {
        vga_set_pixel(x, 200 - TASKBAR_HEIGHT, XP_WHITE);
    }
    
    // Draw Start button
    draw_xp_button(5, 200 - TASKBAR_HEIGHT + 3, 50, 20, "Start", 0);
    
    // Draw window list
    ui_draw_window_list();
    
    // Draw clock
    ui_draw_clock();
}

// Draw XP-style desktop background
void ui_draw_desktop_background(void) {
    // Fill with XP desktop blue
    vga_clear(XP_DESKTOP_BG);
    
    // Draw some "hills" at the bottom (simplified bliss wallpaper)
    for (uint32_t y = 120; y < 180; y++) {
        uint8_t color = XP_LIGHT_GREEN;
        if (y > 150) color = XP_GREEN;
        for (uint32_t x = 0; x < 320; x++) {
            // Simple curved hills
            uint32_t hill_height = 60;
            uint32_t hill_y = 180 - hill_height + (x % 100) / 5;
            if (y >= hill_y && y < 180) {
                vga_set_pixel(x, y, color);
            }
        }
    }
}

// Draw desktop icons
void ui_draw_desktop_icons(void) {
    // My Computer icon
    vga_fill_rect(20, 40, 32, 32, XP_LIGHT_BLUE);
    vga_draw_rect(20, 40, 32, 32, XP_WHITE);
    draw_string(10, 75, "My Computer", XP_WHITE);
    
    // Recycle Bin icon
    vga_fill_rect(20, 90, 32, 32, XP_LIGHT_BLUE);
    vga_draw_rect(20, 90, 32, 32, XP_WHITE);
    draw_string(10, 125, "Recycle Bin", XP_WHITE);
}
