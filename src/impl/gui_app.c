#include "../intf/window.h"
#include "../intf/ui.h"
#include "../intf/graphics.h"
#include "../intf/string.h"
#include "../intf/ui_widgets.h"
#include "../intf/mouse.h"
#include "../user_mode/interfaces/subsystem_interfaces.h"
#include "../user_mode/integral_subsystems/workstation/desktop_manager.h"

// GUI App constants
#define APP_WINDOW_WIDTH 300
#define APP_WINDOW_HEIGHT 220
#define GUI_HEADER_HEIGHT 25
#define TAB_BAR_HEIGHT 20
#define TAB_COUNT 3
#define CONTENT_START_Y (GUI_HEADER_HEIGHT + TAB_BAR_HEIGHT + 10)

// Tab states
typedef enum {
    TAB_INFO,
    TAB_ABOUT,
    TAB_CHANGELOG
} tab_type_t;

static tab_type_t current_tab = TAB_INFO;

// Embedded content
static const char* info_content[] = {
    "GamerOS v1.1 System Information",
    "",
    "Operating System: GamerOS",
    "Version: 1.1",
    "Architecture: x86_64",
    "",
    "Key Features:",
    "- Custom 64-bit kernel",
    "- Layered OS architecture",
    "- VGA graphics (320x200, 256 colors)",
    "- Preemptive multitasking",
    "- Device drivers (keyboard, mouse, RTC)",
    "- GUI framework with windows and widgets",
    "",
    "Memory: Protected allocation",
    "Scheduler: Preemptive round-robin"
};

static const char* changelog_content[] = {
    "GamerOS Development Changelog",
    "",
    "Latest Changes (v1.1.1)",
    "- Enhanced VGA graphics rendering",
    "- Improved UI with Info, About, Changelog tabs",
    "- Updated system information display",
    "- Better font rendering and colors",
    "",
    "v1.1 - The Great Reorganization",
    "- Fixed all critical boot issues",
    "- Added layered OS architecture",
    "- Implemented GUI framework",
    "- Created tabbed interface",
    "- Added system call interface",
    "",
    "v1.0 - Initial Boot Success",
    "- 64-bit long mode support",
    "- Basic memory management",
    "- Interrupt handling system",
    "- VGA graphics driver"
};

static const char* settings_content[] = {
    "System Settings",
    "",
    "Display:",
    "- Resolution: 320x200 (VGA)",
    "- Color Depth: 256 colors",
    "",
    "System:",
    "- Kernel: 64-bit x86_64",
    "- Memory: Protected allocation",
    "- Scheduler: Preemptive multitasking"
};

static const char* about_content[] = {
    "GamerOS v1.1",
    "Build 0.0.0.1600",
    "",
    "Created by: Chosentechies",
    "License: Custom (see LICENSE file)",
    "",
    "A hobby operating system project",
    "built entirely from scratch in C",
    "and x86-64 Assembly."
};

static void set_tab_widget_visibility(
    tab_type_t tab,
    ui_widget_t* info_label, ui_widget_t* info_desc1, ui_widget_t* info_desc2, ui_widget_t* info_feature1, ui_widget_t* info_feature2,
    ui_widget_t* about_label, ui_widget_t* about_desc1, ui_widget_t* about_desc2, ui_widget_t* about_desc3,
    ui_widget_t* changelog_label, ui_widget_t* changelog_desc1, ui_widget_t* changelog_desc2, ui_widget_t* changelog_desc3, ui_widget_t* changelog_desc4) {
    uint8_t show_info = (tab == TAB_INFO);
    uint8_t show_about = (tab == TAB_ABOUT);
    uint8_t show_changelog = (tab == TAB_CHANGELOG);

    if (info_label) ui_set_visible(info_label, show_info);
    if (info_desc1) ui_set_visible(info_desc1, show_info);
    if (info_desc2) ui_set_visible(info_desc2, show_info);
    if (info_feature1) ui_set_visible(info_feature1, show_info);
    if (info_feature2) ui_set_visible(info_feature2, show_info);

    if (about_label) ui_set_visible(about_label, show_about);
    if (about_desc1) ui_set_visible(about_desc1, show_about);
    if (about_desc2) ui_set_visible(about_desc2, show_about);
    if (about_desc3) ui_set_visible(about_desc3, show_about);

    if (changelog_label) ui_set_visible(changelog_label, show_changelog);
    if (changelog_desc1) ui_set_visible(changelog_desc1, show_changelog);
    if (changelog_desc2) ui_set_visible(changelog_desc2, show_changelog);
    if (changelog_desc3) ui_set_visible(changelog_desc3, show_changelog);
    if (changelog_desc4) ui_set_visible(changelog_desc4, show_changelog);
}

// Draw header bar
void draw_header(int win_x, int win_y) {
    // Header background
    vga_fill_rect(win_x + 1, win_y + 1, APP_WINDOW_WIDTH - 2, GUI_HEADER_HEIGHT - 1, COLOR_BLUE);

    // Header title
    draw_string(win_x + 10, win_y + 8, "GamerOS Manager v1.1", COLOR_WHITE);

    // Close button (X)
    vga_fill_rect(win_x + APP_WINDOW_WIDTH - 20, win_y + 5, 15, 15, COLOR_RED);
    draw_string(win_x + APP_WINDOW_WIDTH - 15, win_y + 8, "X", COLOR_WHITE);
}

// Draw tab bar
void draw_tab_bar(int win_x, int win_y) {
    if (TAB_COUNT == 0) return; // Safety check to prevent division by zero
    int tab_width = (APP_WINDOW_WIDTH - 20) / TAB_COUNT;

    for (int i = 0; i < TAB_COUNT; i++) {
        int tab_x = win_x + 10 + i * tab_width;
        int tab_y = win_y + GUI_HEADER_HEIGHT;
        uint8_t bg_color = (i == (int)current_tab) ? COLOR_LIGHT_BLUE : COLOR_DARK_GREY;
        uint8_t text_color = (i == (int)current_tab) ? COLOR_BLACK : COLOR_WHITE;

        // Tab background
        vga_fill_rect(tab_x, tab_y, tab_width - 2, TAB_BAR_HEIGHT, bg_color);
        vga_draw_rect(tab_x, tab_y, tab_width - 2, TAB_BAR_HEIGHT, COLOR_WHITE);

        // Tab text
        const char* tab_names[] = {"Info", "About", "Changelog"};
        int text_x = tab_x + (tab_width - 2 - 6 * strlen(tab_names[i])) / 2; // Center text
        draw_string(text_x, tab_y + 5, tab_names[i], text_color);
    }
}

// Draw content area background
void draw_content_area(int win_x, int win_y) {
    vga_fill_rect(win_x + 5, win_y + CONTENT_START_Y - 5,
                  APP_WINDOW_WIDTH - 10, APP_WINDOW_HEIGHT - CONTENT_START_Y - 5,
                  COLOR_BLACK);
    vga_draw_rect(win_x + 5, win_y + CONTENT_START_Y - 5,
                  APP_WINDOW_WIDTH - 10, APP_WINDOW_HEIGHT - CONTENT_START_Y - 5,
                  COLOR_WHITE);
}

// Draw content based on current tab
void draw_content(int win_x, int win_y) {
    const char** content;
    int line_count;

    switch (current_tab) {
        case TAB_INFO:
            content = info_content;
            line_count = sizeof(info_content) / sizeof(info_content[0]);
            break;
        case TAB_ABOUT:
            content = about_content;
            line_count = sizeof(about_content) / sizeof(about_content[0]);
            break;
        case TAB_CHANGELOG:
            content = changelog_content;
            line_count = sizeof(changelog_content) / sizeof(changelog_content[0]);
            break;
        default:
            return;
    }

    for (int i = 0; i < line_count; i++) {
        draw_string(win_x + 15, win_y + CONTENT_START_Y + i * 10, content[i], COLOR_WHITE);
    }
}

// GUI App main loop
void gui_app_entry() {
    // Initialize desktop manager
    desktop_manager_init();
    
    // Create the XP-style desktop
    desktop_create();

    // Create main application window
    ui_container_t* main_window = ui_create_window(10, 30, APP_WINDOW_WIDTH, APP_WINDOW_HEIGHT, "GamerOS Manager");
    if (!main_window) {
        vga_draw_string(60, 110, "Failed to create window!", COLOR_RED);
        return; // Failed to create window
    }

    vga_draw_string(60, 110, "Window created successfully!", COLOR_GREEN);

    // Create tab buttons (simulating a tab control)
    ui_widget_t* info_button = ui_create_button(15, GUI_HEADER_HEIGHT + 5, 90, 20, "Info");
    ui_widget_t* about_button = ui_create_button(115, GUI_HEADER_HEIGHT + 5, 90, 20, "About");
    ui_widget_t* changelog_button = ui_create_button(215, GUI_HEADER_HEIGHT + 5, 90, 20, "Changelog");

    // Create content area panel
    ui_container_t* content_panel = ui_create_panel(10, CONTENT_START_Y - 5,
                                                   APP_WINDOW_WIDTH - 20, APP_WINDOW_HEIGHT - CONTENT_START_Y - 5);

    // Add widgets to main window
    if (info_button) ui_add_child(main_window, info_button);
    if (about_button) ui_add_child(main_window, about_button);
    if (changelog_button) ui_add_child(main_window, changelog_button);
    if (content_panel) ui_add_child(main_window, (ui_widget_t*)content_panel);

    // Create content widgets for different tabs
    ui_widget_t* info_label = ui_create_label(10, 10, 250, 20, "GamerOS System Information");
    ui_widget_t* info_desc1 = ui_create_label(10, 35, 280, 15, "Operating System: GamerOS v1.1");
    ui_widget_t* info_desc2 = ui_create_label(10, 55, 280, 15, "Architecture: x86_64");
    ui_widget_t* info_feature1 = ui_create_label(10, 80, 280, 15, "Features: VGA graphics, GUI framework");
    ui_widget_t* info_feature2 = ui_create_label(10, 95, 280, 15, "Multitasking, device drivers");

    ui_widget_t* about_label = ui_create_label(10, 10, 280, 20, "About GamerOS");
    ui_widget_t* about_desc1 = ui_create_label(10, 35, 280, 15, "Created by: Chosentechies");
    ui_widget_t* about_desc2 = ui_create_label(10, 55, 280, 15, "A hobby operating system project");
    ui_widget_t* about_desc3 = ui_create_label(10, 75, 280, 15, "Built entirely from scratch in C");

    ui_widget_t* changelog_label = ui_create_label(10, 10, 280, 20, "GamerOS Development Changelog");
    ui_widget_t* changelog_desc1 = ui_create_label(10, 35, 280, 15, "v1.1.1 - UI Improvements");
    ui_widget_t* changelog_desc2 = ui_create_label(10, 55, 280, 15, "- Enhanced VGA graphics rendering");
    ui_widget_t* changelog_desc3 = ui_create_label(10, 75, 280, 15, "- Added Info, About, Changelog tabs");
    ui_widget_t* changelog_desc4 = ui_create_label(10, 95, 280, 15, "- Improved system information display");

    // Add content widgets to content panel
    if (content_panel) {
        ui_add_child(content_panel, info_label);
        ui_add_child(content_panel, info_desc1);
        ui_add_child(content_panel, info_desc2);
        ui_add_child(content_panel, info_feature1);
        ui_add_child(content_panel, info_feature2);

        ui_add_child(content_panel, about_label);
        ui_add_child(content_panel, about_desc1);
        ui_add_child(content_panel, about_desc2);
        ui_add_child(content_panel, about_desc3);

        ui_add_child(content_panel, changelog_label);
        ui_add_child(content_panel, changelog_desc1);
        ui_add_child(content_panel, changelog_desc2);
        ui_add_child(content_panel, changelog_desc3);
        ui_add_child(content_panel, changelog_desc4);
    }

    set_tab_widget_visibility(current_tab,
        info_label, info_desc1, info_desc2, info_feature1, info_feature2,
        about_label, about_desc1, about_desc2, about_desc3,
        changelog_label, changelog_desc1, changelog_desc2, changelog_desc3, changelog_desc4);

    uint8_t last_mouse_buttons = 0;

    while (1) {
        // Update desktop (draws background, taskbar, handles input)
        desktop_update();

        // Draw app shell window.
        int win_x = 10;
        int win_y = 30;
        vga_fill_rect(win_x, win_y, APP_WINDOW_WIDTH, APP_WINDOW_HEIGHT, COLOR_DARK_GREY);
        vga_draw_rect(win_x, win_y, APP_WINDOW_WIDTH, APP_WINDOW_HEIGHT, COLOR_WHITE);
        draw_header(win_x, win_y);
        draw_tab_bar(win_x, win_y);
        draw_content_area(win_x, win_y);
        draw_content(win_x, win_y);
        
        // Process mouse input for close button and tab clicks.
        mouse_state_t mouse = mouse_get_state();
        uint8_t pressed = mouse.buttons & (uint8_t)(~last_mouse_buttons);
        if (pressed & MOUSE_BTN_LEFT) {
            int close_x = win_x + APP_WINDOW_WIDTH - 20;
            int close_y = win_y + 5;
            if (mouse.x >= close_x && mouse.x < close_x + 15 &&
                mouse.y >= close_y && mouse.y < close_y + 15) {
                break;
            }

            int tab_width = (APP_WINDOW_WIDTH - 20) / TAB_COUNT;
            int tab_y = win_y + GUI_HEADER_HEIGHT;
            if (mouse.y >= tab_y && mouse.y < tab_y + TAB_BAR_HEIGHT) {
                for (int i = 0; i < TAB_COUNT; i++) {
                    int tab_x = win_x + 10 + i * tab_width;
                    if (mouse.x >= tab_x && mouse.x < tab_x + tab_width - 2) {
                        current_tab = (tab_type_t)i;
                        set_tab_widget_visibility(current_tab,
                            info_label, info_desc1, info_desc2, info_feature1, info_feature2,
                            about_label, about_desc1, about_desc2, about_desc3,
                            changelog_label, changelog_desc1, changelog_desc2, changelog_desc3, changelog_desc4);
                        break;
                    }
                }
            }
        }
        last_mouse_buttons = mouse.buttons;

        // Keyboard events: 1/2/3 switch tabs, ESC exits app.
        while (keyboard_has_input()) {
            char key = keyboard_getchar();
            if (key == 27) {
                return;
            } else if (key == '1') {
                current_tab = TAB_INFO;
            } else if (key == '2') {
                current_tab = TAB_ABOUT;
            } else if (key == '3') {
                current_tab = TAB_CHANGELOG;
            } else {
                continue;
            }
            set_tab_widget_visibility(current_tab,
                info_label, info_desc1, info_desc2, info_feature1, info_feature2,
                about_label, about_desc1, about_desc2, about_desc3,
                changelog_label, changelog_desc1, changelog_desc2, changelog_desc3, changelog_desc4);
        }

        // Draw the mouse cursor at current position.
        vga_draw_bitmap_cursor(mouse.x, mouse.y);
        __asm__ volatile("int $0x20");
    }

    // Cleanup (in a real application, this would be called on exit)
    // ui_destroy_widget((ui_widget_t*)main_window);
}

// TODO: Add more interactive widgets (text input, checkboxes, dropdowns)
// TODO: Implement dynamic content loading instead of hardcoded strings
// TODO: Add window resizing and movement capabilities
// TODO: Implement proper application lifecycle management (start, pause, resume, exit)
