// src/impl/kernel/main.c - ULTRA-SIMPLIFIED TEST VERSION

#include "../../intf/kernel.h"
#include "../../intf/stdint.h"
#include "../../intf/graphics.h"
#include "../../intf/ports.h"
#include "../../intf/mouse.h"

// Special key codes from keyboard driver
#define KEY_LEFT_ARROW   0x80
#define KEY_RIGHT_ARROW  0x81
#define KEY_UP_ARROW     0x82
#define KEY_DOWN_ARROW   0x83
#define KEY_HOME         0x84
#define KEY_END          0x85
#define KEY_PAGE_UP      0x86
#define KEY_PAGE_DOWN    0x87
#define KEY_INSERT       0x88
#define KEY_DELETE       0x89

// UI State
typedef enum {
    TAB_HOME = 0,
    TAB_CHANGELOG = 1,
    TAB_SETTINGS = 2
} tab_t;

volatile tab_t current_tab = TAB_HOME;

// Mouse state tracking
static int32_t last_mouse_x = 160;
static int32_t last_mouse_y = 100;

// External keyboard functions from driver
extern int keyboard_has_char(void);
extern char keyboard_read_char(void);
extern void keyboard_init(void);

// External mouse functions from driver
extern void mouse_init(void);
extern int32_t mouse_get_x(void);
extern int32_t mouse_get_y(void);
extern uint8_t mouse_get_buttons(void);

// Forward declaration of draw_ui
void draw_ui(tab_t active_tab);

// Full keyboard input handler with driver support
void handle_keyboard_input(void) {
    // Poll for keyboard input from driver buffer
    while (keyboard_has_char()) {
        char key = keyboard_read_char();
        
        if (key == 0) continue;
        
        // Handle different key codes and navigation
        switch (key) {
            // Left/Up navigation keys
            case KEY_LEFT_ARROW:
            case KEY_UP_ARROW:
            case 'a':  // Fallback WASD support
            case 'A':
            case 'w':
            case 'W':
                if (current_tab > 0) {
                    current_tab--;
                    draw_ui(current_tab);
                }
                break;
                
            // Right/Down navigation keys
            case KEY_RIGHT_ARROW:
            case KEY_DOWN_ARROW:
            case 'd':  // Fallback WASD support
            case 'D':
            case 's':
            case 'S':
            case '\t':  // Tab key
                if (current_tab < TAB_SETTINGS) {
                    current_tab++;
                    draw_ui(current_tab);
                }
                break;
                
            // Jump to Home tab
            case KEY_HOME:
                if (current_tab != TAB_HOME) {
                    current_tab = TAB_HOME;
                    draw_ui(current_tab);
                }
                break;
                
            // Jump to Settings tab
            case KEY_END:
                if (current_tab != TAB_SETTINGS) {
                    current_tab = TAB_SETTINGS;
                    draw_ui(current_tab);
                }
                break;
                
            // Page navigation
            case KEY_PAGE_UP:
                if (current_tab > 0) {
                    current_tab--;
                    draw_ui(current_tab);
                }
                break;
                
            case KEY_PAGE_DOWN:
                if (current_tab < TAB_SETTINGS) {
                    current_tab++;
                    draw_ui(current_tab);
                }
                break;
                
            // Confirm/activate key (no-op for now, tab switching is automatic)
            case '\n':
            case '\r':
            case ' ':   // Space
                break;
        }
    }
}

// Mouse input handler with cursor rendering
void handle_mouse_input(void) {
    // Get current mouse position
    int32_t mouse_x = mouse_get_x();
    int32_t mouse_y = mouse_get_y();
    uint8_t buttons = mouse_get_buttons();
    
    // Check if mouse moved
    if (mouse_x != last_mouse_x || mouse_y != last_mouse_y) {
        // Erase old cursor
        vga_erase_mouse_area(last_mouse_x, last_mouse_y);
        
        // Draw new cursor
        vga_draw_mouse_arrow(mouse_x, mouse_y, 0x0F);  // White arrow cursor
        
        // Update last position
        last_mouse_x = mouse_x;
        last_mouse_y = mouse_y;
    }
    
    // Handle mouse clicks on tabs (VGA Mode 12h 640x480)
    if (buttons & MOUSE_LEFT_BUTTON) {
        // Check if click is within tab area (y: 58-93, x varies by tab)
        if (mouse_y >= 58 && mouse_y <= 93) {
            // HOME tab: x: 10-210
            if (mouse_x >= 10 && mouse_x < 210) {
                if (current_tab != TAB_HOME) {
                    current_tab = TAB_HOME;
                    draw_ui(current_tab);
                }
            }
            // CHANGELOG tab: x: 220-420
            else if (mouse_x >= 220 && mouse_x < 420) {
                if (current_tab != TAB_CHANGELOG) {
                    current_tab = TAB_CHANGELOG;
                    draw_ui(current_tab);
                }
            }
            // SETTINGS tab: x: 430-630
            else if (mouse_x >= 430 && mouse_x < 630) {
                if (current_tab != TAB_SETTINGS) {
                    current_tab = TAB_SETTINGS;
                    draw_ui(current_tab);
                }
            }
        }
    }
}

void draw_ui(tab_t active_tab) {
    vga_clear(0x01);  // Blue background

    // LARGE Title bar for 640x480
    vga_fill_rect(0, 0, 640, 50, 0x09);  // Bright blue
    vga_draw_string(20, 18, "GamerOS 1.0 - VGA 12h (640x480) Edition", 0x0F);  // White text

    // Decorative top border
    vga_draw_horizontal_line(0, 50, 640, 0x0E);  // Yellow line
    vga_draw_horizontal_line(0, 51, 640, 0x0E);  // Double line

    // Tab bar
    vga_fill_rect(0, 55, 640, 40, 0x07);  // Light gray background
    vga_draw_horizontal_line(0, 95, 640, 0x0F);  // Bottom border

    // Calculate tab positions - LARGER tabs for 640x480
    int tab_width = 200;
    int tab_height = 35;
    int tab_y = 58;
    int spacing = 10;
    
    // Home tab
    int home_x = 10;
    if (active_tab == TAB_HOME) {
        vga_fill_rect(home_x, tab_y, tab_width, tab_height, 0x0A);  // Bright green
        vga_draw_rect(home_x, tab_y, tab_width, tab_height, 0x0F);
        vga_draw_string(home_x + 70, tab_y + 12, "HOME", 0x00);  // Black text
    } else {
        vga_fill_rect(home_x, tab_y, tab_width, tab_height, 0x07);
        vga_draw_rect(home_x, tab_y, tab_width, tab_height, 0x0F);
        vga_draw_string(home_x + 70, tab_y + 12, "HOME", 0x00);
    }

    // Changelog tab
    int changelog_x = home_x + tab_width + spacing;
    if (active_tab == TAB_CHANGELOG) {
        vga_fill_rect(changelog_x, tab_y, tab_width, tab_height, 0x0A);
        vga_draw_rect(changelog_x, tab_y, tab_width, tab_height, 0x0F);
        vga_draw_string(changelog_x + 50, tab_y + 12, "CHANGELOG", 0x00);
    } else {
        vga_fill_rect(changelog_x, tab_y, tab_width, tab_height, 0x07);
        vga_draw_rect(changelog_x, tab_y, tab_width, tab_height, 0x0F);
        vga_draw_string(changelog_x + 50, tab_y + 12, "CHANGELOG", 0x00);
    }

    // Settings tab
    int settings_x = changelog_x + tab_width + spacing;
    if (active_tab == TAB_SETTINGS) {
        vga_fill_rect(settings_x, tab_y, tab_width, tab_height, 0x0A);
        vga_draw_rect(settings_x, tab_y, tab_width, tab_height, 0x0F);
        vga_draw_string(settings_x + 60, tab_y + 12, "SETTINGS", 0x00);
    } else {
        vga_fill_rect(settings_x, tab_y, tab_width, tab_height, 0x07);
        vga_draw_rect(settings_x, tab_y, tab_width, tab_height, 0x0F);
        vga_draw_string(settings_x + 60, tab_y + 12, "SETTINGS", 0x00);
    }

    // HUGE content area for 640x480
    int content_x = 10;
    int content_y = 105;
    int content_width = 620;
    int content_height = 350;
    
    // Content area with thick borders
    vga_draw_rect(content_x, content_y, content_width, content_height, 0x0F);
    vga_draw_rect(content_x + 1, content_y + 1, content_width - 2, content_height - 2, 0x0F);
    vga_fill_rect(content_x + 2, content_y + 2, content_width - 4, content_height - 4, 0x00);

    // Content text
    int text_x = content_x + 15;
    int text_y = content_y + 15;
    int line_height = 9;

    if (active_tab == TAB_HOME) {
        vga_draw_string(text_x, text_y, "Welcome to GamerOS 1.0", 0x0E);
        vga_draw_string(text_x, text_y + line_height * 2, "Professional Retro Operating System", 0x0F);
        
        vga_draw_string(text_x, text_y + line_height * 4, "System Information:", 0x0A);
        vga_draw_string(text_x, text_y + line_height * 5, "Architecture: x86-64 | Processor: Custom", 0x02);
        vga_draw_string(text_x, text_y + line_height * 6, "Display: 640x480x16 colors", 0x02);
        vga_draw_string(text_x, text_y + line_height * 7, "Graphics: VGA Mode 12h (Planar Mode)", 0x02);
        vga_draw_string(text_x, text_y + line_height * 8, "Framebuffer: 0xA0000 (64KB)", 0x02);
        
        vga_draw_string(text_x, text_y + line_height * 10, "Key Features:", 0x0A);
        vga_draw_string(text_x, text_y + line_height * 11, "> Real-time graphics rendering with Bresenham algorithms", 0x0E);
        vga_draw_string(text_x, text_y + line_height * 12, "> Professional retro UI design with Windows 1.0 styling", 0x0E);
        vga_draw_string(text_x, text_y + line_height * 13, "> Full mouse support with interactive cursor", 0x0E);
        vga_draw_string(text_x, text_y + line_height * 14, "> Keyboard input with arrow keys and tab navigation", 0x0E);
        vga_draw_string(text_x, text_y + line_height * 15, "> Custom microkernel architecture", 0x0E);
        vga_draw_string(text_x, text_y + line_height * 16, "> VGA planar mode for efficient memory usage", 0x0E);
        
    } else if (active_tab == TAB_CHANGELOG) {
        vga_draw_string(text_x, text_y, "GamerOS Version 1.0 - Initial Release", 0x0E);
        vga_draw_string(text_x, text_y + line_height * 2, "Complete Feature Set", 0x0F);
        
        vga_draw_string(text_x, text_y + line_height * 4, "Graphics Subsystem:", 0x0A);
        vga_draw_string(text_x, text_y + line_height * 5, "+ vga_draw_line - Bresenham's line algorithm for fast line drawing", 0x02);
        vga_draw_string(text_x, text_y + line_height * 6, "+ vga_draw_circle - Midpoint circle algorithm for smooth curves", 0x02);
        vga_draw_string(text_x, text_y + line_height * 7, "+ vga_fill_circle - Hardware-optimized filled circles", 0x02);
        vga_draw_string(text_x, text_y + line_height * 8, "+ vga_draw_rect - Rectangle drawing and filling", 0x02);
        vga_draw_string(text_x, text_y + line_height * 9, "+ vga_set_mode - Support for both Mode 13h and Mode 12h", 0x02);
        
        vga_draw_string(text_x, text_y + line_height * 11, "User Interface System:", 0x0A);
        vga_draw_string(text_x, text_y + line_height * 12, "+ Tab-based interface with click navigation", 0x02);
        vga_draw_string(text_x, text_y + line_height * 13, "+ Professional color scheme with 16-color palette", 0x02);
        vga_draw_string(text_x, text_y + line_height * 14, "+ Retro Windows 1.0 styling with double borders", 0x02);
        
    } else if (active_tab == TAB_SETTINGS) {
        vga_draw_string(text_x, text_y, "System Settings & Configuration", 0x0E);
        vga_draw_string(text_x, text_y + line_height * 2, "Display & Hardware Setup", 0x0F);
        
        vga_draw_string(text_x, text_y + line_height * 4, "Video Mode Configuration:", 0x0A);
        vga_draw_string(text_x, text_y + line_height * 5, "Current: VGA Mode 12h (640x480x16 Planar)", 0x02);
        vga_draw_string(text_x, text_y + line_height * 6, "Framebuffer: 0xA0000 (64 KB memory)", 0x02);
        vga_draw_string(text_x, text_y + line_height * 7, "Color Depth: 4 planes x 4 bits per pixel", 0x02);
        vga_draw_string(text_x, text_y + line_height * 8, "Memory: Sequential/Planar mode", 0x02);
        
        vga_draw_string(text_x, text_y + line_height * 10, "System Architecture:", 0x0A);
        vga_draw_string(text_x, text_y + line_height * 11, "Processor: x86-64 (Long Mode)", 0x02);
        vga_draw_string(text_x, text_y + line_height * 12, "Bootloader: GRUB 2 Multiboot", 0x02);
        vga_draw_string(text_x, text_y + line_height * 13, "Kernel: Custom Microkernel", 0x02);
        vga_draw_string(text_x, text_y + line_height * 14, "Version: 1.0 Professional Edition", 0x02);
        vga_draw_string(text_x, text_y + line_height * 15, "Input: Full Keyboard + Mouse Support", 0x02);
    }

    // Status bar
    vga_fill_rect(0, 465, 640, 15, 0x07);
    vga_draw_string(10, 470, "[ Ready ]  GamerOS v1.0 VGA Mode 12h", 0x00);
}

void kernel_main(void) {
    // Initialize graphics mode - VGA 12h (640x480x16)
    vga_set_mode(VGA_MODE_12H);

    // Quick test - fill screen with white to verify mode 12h is working
    vga_clear(0x0F);  // Fill with white (color 15)
    
    // Small delay
    for (volatile int i = 0; i < 100000; i++) {
        __asm__("nop");
    }

    // Initialize keyboard driver for input handling
    keyboard_init();
    
    // Initialize mouse driver for input and cursor
    mouse_init();

    // Draw initial UI
    current_tab = TAB_HOME;
    draw_ui(current_tab);
    
    // Draw initial mouse cursor at center screen
    vga_draw_mouse_arrow(320, 240, 0x0F);

    // Main event loop - process both keyboard and mouse input
    while (1) {
        // Poll and handle keyboard input (arrow keys, tab, home, end, etc.)
        handle_keyboard_input();
        
        // Poll and handle mouse input (movement and clicks)
        handle_mouse_input();
        
        // Small delay to prevent busy-waiting
        for (volatile int i = 0; i < 1000; i++) {
            __asm__("nop");
        }
    }
}

// TODO: Implement proper interrupt-driven keyboard handling
// TODO: Implement proper event system for UI interactions
// TODO: Add more settings options
// TODO: Add system information display