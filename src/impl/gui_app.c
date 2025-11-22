#include "../intf/window.h"
#include "../intf/ui.h"
#include "../intf/graphics.h"
#include "../intf/string.h"
#include "../intf/ui_widgets.h"

// GUI App constants
#define APP_WINDOW_WIDTH 300
#define APP_WINDOW_HEIGHT 180
#define GUI_HEADER_HEIGHT 25
#define TAB_BAR_HEIGHT 20
#define TAB_COUNT 4
#define CONTENT_START_Y (GUI_HEADER_HEIGHT + TAB_BAR_HEIGHT + 10)

// Tab states
typedef enum {
    TAB_HOME,
    TAB_CHANGELOG,
    TAB_SETTINGS,
    TAB_ABOUT
} tab_type_t;

static tab_type_t current_tab = TAB_HOME;

// Embedded content
static const char* home_content[] = {
    "Welcome to GamerOS Manager",
    "",
    "A simple x86_64 hobby operating system",
    "with Windows 11-inspired desktop UI.",
    "",
    "Key Features:",
    "- Custom 64-bit kernel",
    "- Graphical desktop environment",
    "- Hardware device drivers",
    "- Multitasking scheduler"
};

static const char* changelog_content[] = {
    "Build 0.0.0.1600 - Complete OS Overhaul",
    "",
    "Major Changes:",
    "- Fixed critical QEMU boot issues",
    "- Reorganized codebase into subsystems",
    "- Implemented system calls (int 0x80)",
    "- Added comprehensive memory protection",
    "- Enhanced device driver framework",
    "- Improved filesystem with persistence"
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
        const char* tab_names[] = {"Home", "Changelog", "Settings", "About"};
        int text_x = tab_x + (tab_width - 2 - 6 * 4) / 2; // Center text
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
        case TAB_HOME:
            content = home_content;
            line_count = sizeof(home_content) / sizeof(home_content[0]);
            break;
        case TAB_CHANGELOG:
            content = changelog_content;
            line_count = sizeof(changelog_content) / sizeof(changelog_content[0]);
            break;
        case TAB_SETTINGS:
            content = settings_content;
            line_count = sizeof(settings_content) / sizeof(settings_content[0]);
            break;
        case TAB_ABOUT:
            content = about_content;
            line_count = sizeof(about_content) / sizeof(about_content[0]);
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
    // Initialize UI widgets system
    ui_widgets_init();

    // Simple test: fill screen with blue to show GUI is running
    vga_set_desktop_background();
    vga_fill_rect(50, 50, 200, 100, 0xFF0000FF); // Blue rectangle
    vga_draw_string(60, 70, "GUI Application Running!", 0xFFFFFFFF);
    vga_draw_string(60, 90, "Widget Framework Active", 0xFFFFFFFF);

    // Create main application window
    ui_container_t* main_window = ui_create_window(10, 30, APP_WINDOW_WIDTH, APP_WINDOW_HEIGHT, "GamerOS Manager");
    if (!main_window) {
        vga_draw_string(60, 110, "Failed to create window!", 0xFFFF0000);
        return; // Failed to create window
    }

    vga_draw_string(60, 110, "Window created successfully!", 0xFF00FF00);

    // Create tab buttons (simulating a tab control)
    ui_widget_t* home_button = ui_create_button(15, GUI_HEADER_HEIGHT + 5, 80, 20, "Home");
    ui_widget_t* changelog_button = ui_create_button(100, GUI_HEADER_HEIGHT + 5, 80, 20, "Changelog");
    ui_widget_t* settings_button = ui_create_button(185, GUI_HEADER_HEIGHT + 5, 80, 20, "Settings");
    ui_widget_t* about_button = ui_create_button(270, GUI_HEADER_HEIGHT + 5, 80, 20, "About");

    // Create content area panel
    ui_container_t* content_panel = ui_create_panel(10, CONTENT_START_Y - 5,
                                                   APP_WINDOW_WIDTH - 20, APP_WINDOW_HEIGHT - CONTENT_START_Y - 5);

    // Add widgets to main window
    if (home_button) ui_add_child(main_window, home_button);
    if (changelog_button) ui_add_child(main_window, changelog_button);
    if (settings_button) ui_add_child(main_window, settings_button);
    if (about_button) ui_add_child(main_window, about_button);
    if (content_panel) ui_add_child(main_window, (ui_widget_t*)content_panel);

    // Create content widgets for different tabs
    ui_widget_t* home_label = ui_create_label(10, 10, 250, 20, "Welcome to GamerOS Manager");
    ui_widget_t* home_desc1 = ui_create_label(10, 35, 280, 15, "A simple x86_64 hobby operating system");
    ui_widget_t* home_desc2 = ui_create_label(10, 55, 280, 15, "with Windows 11-inspired desktop UI.");
    ui_widget_t* home_feature1 = ui_create_label(10, 80, 280, 15, "Features: Custom kernel, multitasking,");
    ui_widget_t* home_feature2 = ui_create_label(10, 95, 280, 15, "device drivers, and GUI framework.");

    ui_widget_t* changelog_label = ui_create_label(10, 10, 280, 20, "Build 0.0.0.1600 - Complete OS Overhaul");
    ui_widget_t* changelog_desc = ui_create_label(10, 35, 280, 15, "Fixed critical boot issues, added UI framework");

    ui_widget_t* settings_label = ui_create_label(10, 10, 200, 20, "System Settings");
    ui_widget_t* settings_res = ui_create_label(10, 35, 200, 15, "Resolution: 320x200 (VGA)");
    ui_widget_t* settings_color = ui_create_label(10, 55, 200, 15, "Color Depth: 256 colors");
    ui_widget_t* settings_kernel = ui_create_label(10, 75, 200, 15, "Kernel: 64-bit x86_64");

    ui_widget_t* about_label = ui_create_label(10, 10, 200, 20, "GamerOS v1.1");
    ui_widget_t* about_build = ui_create_label(10, 35, 200, 15, "Build 0.0.0.1600");
    ui_widget_t* about_author = ui_create_label(10, 55, 200, 15, "Created by: Chosentechies");

    // Add content widgets to content panel
    if (content_panel) {
        ui_add_child(content_panel, home_label);
        ui_add_child(content_panel, home_desc1);
        ui_add_child(content_panel, home_desc2);
        ui_add_child(content_panel, home_feature1);
        ui_add_child(content_panel, home_feature2);

        ui_add_child(content_panel, changelog_label);
        ui_add_child(content_panel, changelog_desc);

        ui_add_child(content_panel, settings_label);
        ui_add_child(content_panel, settings_res);
        ui_add_child(content_panel, settings_color);
        ui_add_child(content_panel, settings_kernel);

        ui_add_child(content_panel, about_label);
        ui_add_child(content_panel, about_build);
        ui_add_child(content_panel, about_author);
    }

    // Initially show home tab content
    if (home_label) ui_set_visible(home_label, 1);
    if (home_desc1) ui_set_visible(home_desc1, 1);
    if (home_desc2) ui_set_visible(home_desc2, 1);
    if (home_feature1) ui_set_visible(home_feature1, 1);
    if (home_feature2) ui_set_visible(home_feature2, 1);

    // Hide other tab content initially
    if (changelog_label) ui_set_visible(changelog_label, 0);
    if (changelog_desc) ui_set_visible(changelog_desc, 0);
    if (settings_label) ui_set_visible(settings_label, 0);
    if (settings_res) ui_set_visible(settings_res, 0);
    if (settings_color) ui_set_visible(settings_color, 0);
    if (settings_kernel) ui_set_visible(settings_kernel, 0);
    if (about_label) ui_set_visible(about_label, 0);
    if (about_build) ui_set_visible(about_build, 0);
    if (about_author) ui_set_visible(about_author, 0);

    while (1) {
        // Clear screen and set desktop background
        vga_set_desktop_background();

        // Render the main window and all its widgets
        ui_render_container(main_window);

        // Simple tab switching simulation (in real OS, would handle mouse/keyboard input)
        static int counter = 0;
        counter++;
        if (counter % 3000000 == 0) { // Slow cycle for demo
            // Switch tabs
            current_tab = (current_tab + 1) % TAB_COUNT;

            // Update visibility of content widgets based on current tab
            uint8_t show_home = (current_tab == TAB_HOME);
            uint8_t show_changelog = (current_tab == TAB_CHANGELOG);
            uint8_t show_settings = (current_tab == TAB_SETTINGS);
            uint8_t show_about = (current_tab == TAB_ABOUT);

            if (home_label) ui_set_visible(home_label, show_home);
            if (home_desc1) ui_set_visible(home_desc1, show_home);
            if (home_desc2) ui_set_visible(home_desc2, show_home);
            if (home_feature1) ui_set_visible(home_feature1, show_home);
            if (home_feature2) ui_set_visible(home_feature2, show_home);

            if (changelog_label) ui_set_visible(changelog_label, show_changelog);
            if (changelog_desc) ui_set_visible(changelog_desc, show_changelog);

            if (settings_label) ui_set_visible(settings_label, show_settings);
            if (settings_res) ui_set_visible(settings_res, show_settings);
            if (settings_color) ui_set_visible(settings_color, show_settings);
            if (settings_kernel) ui_set_visible(settings_kernel, show_settings);

            if (about_label) ui_set_visible(about_label, show_about);
            if (about_build) ui_set_visible(about_build, show_about);
            if (about_author) ui_set_visible(about_author, show_about);
        }

        // Yield to scheduler periodically
        if (counter % 100000 == 0) {
            __asm__ volatile("int $0x20");
        }
    }

    // Cleanup (in a real application, this would be called on exit)
    // ui_destroy_widget((ui_widget_t*)main_window);
}