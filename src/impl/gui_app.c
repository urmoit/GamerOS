#include "../intf/window.h"
#include "../intf/ui.h"
#include "../intf/graphics.h"
#include "../intf/string.h"
#include "../intf/ui_widgets.h"
#include "../user_mode/interfaces/subsystem_interfaces.h"

// GUI App constants
#define APP_WINDOW_WIDTH 300
#define APP_WINDOW_HEIGHT 220
#define GUI_HEADER_HEIGHT 25
#define TAB_BAR_HEIGHT 20
#define TAB_COUNT 2
#define CONTENT_START_Y (GUI_HEADER_HEIGHT + TAB_BAR_HEIGHT + 10)

// Tab states
typedef enum {
    TAB_HOME,
    TAB_CHANGELOG
} tab_type_t;

static tab_type_t current_tab = TAB_HOME;

// Embedded content
static const char* home_content[] = {
    "Welcome to GamerOS v1.1!",
    "",
    "Fun Facts About Your OS:",
    "- Built from scratch in C & Assembly",
    "- 64-bit kernel with custom paging",
    "- Runs entirely in memory (no HDD needed!)",
    "- Supports VGA graphics mode 13h",
    "- Has a preemptive scheduler",
    "",
    "Did you know?",
    "- This window is 300x220 pixels",
    "- Font uses 8x8 bitmap characters",
    "- Tabs cycle every 3 million instructions",
    "- OS boots in under 2 seconds!"
};

static const char* changelog_content[] = {
    "GamerOS Development Changelog",
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
    "- VGA graphics driver",
    "",
    "Future Plans:",
    "- User input handling",
    "- Network stack",
    "- More compatibility layers"
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
        const char* tab_names[] = {"Home", "Changelog"};
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
        case TAB_HOME:
            content = home_content;
            line_count = sizeof(home_content) / sizeof(home_content[0]);
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
    // Create desktop using workstation interface
    workstation_create_desktop();

    // Simple test: show GUI is running
    // Note: Using 8-bit palette indices for VGA mode 13h
    // vga_fill_rect(50, 50, 200, 100, COLOR_BLUE); // Blue rectangle (commented out - bars are drawn by desktop)
    // vga_draw_string(60, 70, "GUI Application Running!", COLOR_WHITE);
    // vga_draw_string(60, 90, "Layered Architecture Active", COLOR_WHITE);

    // Create main application window
    ui_container_t* main_window = ui_create_window(10, 30, APP_WINDOW_WIDTH, APP_WINDOW_HEIGHT, "GamerOS Manager");
    if (!main_window) {
        vga_draw_string(60, 110, "Failed to create window!", COLOR_RED);
        return; // Failed to create window
    }

    vga_draw_string(60, 110, "Window created successfully!", COLOR_GREEN);

    // Create tab buttons (simulating a tab control)
    ui_widget_t* home_button = ui_create_button(15, GUI_HEADER_HEIGHT + 5, 130, 20, "Home");
    ui_widget_t* changelog_button = ui_create_button(155, GUI_HEADER_HEIGHT + 5, 130, 20, "Changelog");

    // Create content area panel
    ui_container_t* content_panel = ui_create_panel(10, CONTENT_START_Y - 5,
                                                   APP_WINDOW_WIDTH - 20, APP_WINDOW_HEIGHT - CONTENT_START_Y - 5);

    // Add widgets to main window
    if (home_button) ui_add_child(main_window, home_button);
    if (changelog_button) ui_add_child(main_window, changelog_button);
    if (content_panel) ui_add_child(main_window, (ui_widget_t*)content_panel);

    // Create content widgets for different tabs
    ui_widget_t* home_label = ui_create_label(10, 10, 250, 20, "Welcome to GamerOS Manager");
    ui_widget_t* home_desc1 = ui_create_label(10, 35, 280, 15, "A simple x86_64 hobby operating system");
    ui_widget_t* home_desc2 = ui_create_label(10, 55, 280, 15, "with Windows 11-inspired desktop UI.");
    ui_widget_t* home_feature1 = ui_create_label(10, 80, 280, 15, "Features: Custom kernel, multitasking,");
    ui_widget_t* home_feature2 = ui_create_label(10, 95, 280, 15, "device drivers, and GUI framework.");

    ui_widget_t* changelog_label = ui_create_label(10, 10, 280, 20, "GamerOS Development Changelog");
    ui_widget_t* changelog_desc1 = ui_create_label(10, 35, 280, 15, "v1.1 - The Great Reorganization");
    ui_widget_t* changelog_desc2 = ui_create_label(10, 55, 280, 15, "- Fixed all critical boot issues");
    ui_widget_t* changelog_desc3 = ui_create_label(10, 75, 280, 15, "- Added layered OS architecture");
    ui_widget_t* changelog_desc4 = ui_create_label(10, 95, 280, 15, "- Implemented GUI framework");
    ui_widget_t* changelog_desc5 = ui_create_label(10, 115, 280, 15, "- Created tabbed interface");

    // Add content widgets to content panel
    if (content_panel) {
        ui_add_child(content_panel, home_label);
        ui_add_child(content_panel, home_desc1);
        ui_add_child(content_panel, home_desc2);
        ui_add_child(content_panel, home_feature1);
        ui_add_child(content_panel, home_feature2);

        ui_add_child(content_panel, changelog_label);
        ui_add_child(content_panel, changelog_desc1);
        ui_add_child(content_panel, changelog_desc2);
        ui_add_child(content_panel, changelog_desc3);
        ui_add_child(content_panel, changelog_desc4);
        ui_add_child(content_panel, changelog_desc5);
    }

    // Initially show home tab content
    if (home_label) ui_set_visible(home_label, 1);
    if (home_desc1) ui_set_visible(home_desc1, 1);
    if (home_desc2) ui_set_visible(home_desc2, 1);
    if (home_feature1) ui_set_visible(home_feature1, 1);
    if (home_feature2) ui_set_visible(home_feature2, 1);

    // Hide changelog tab content initially
    if (changelog_label) ui_set_visible(changelog_label, 0);
    if (changelog_desc1) ui_set_visible(changelog_desc1, 0);
    if (changelog_desc2) ui_set_visible(changelog_desc2, 0);
    if (changelog_desc3) ui_set_visible(changelog_desc3, 0);
    if (changelog_desc4) ui_set_visible(changelog_desc4, 0);
    if (changelog_desc5) ui_set_visible(changelog_desc5, 0);

    while (1) {
        // Redraw the four colored bars directly to framebuffer (desktop background)
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

        // Temporarily disable UI rendering to test if it's overwriting the bars
        // ui_render_container(main_window);

        // Simple tab switching simulation (in real OS, would handle mouse/keyboard input)
        static int counter = 0;
        counter++;
        if (counter % 3000000 == 0) { // Slow cycle for demo
            // Switch tabs
            current_tab = (current_tab + 1) % TAB_COUNT;

            // Update visibility of content widgets based on current tab
            uint8_t show_home = (current_tab == TAB_HOME);
            uint8_t show_changelog = (current_tab == TAB_CHANGELOG);

            if (home_label) ui_set_visible(home_label, show_home);
            if (home_desc1) ui_set_visible(home_desc1, show_home);
            if (home_desc2) ui_set_visible(home_desc2, show_home);
            if (home_feature1) ui_set_visible(home_feature1, show_home);
            if (home_feature2) ui_set_visible(home_feature2, show_home);

            if (changelog_label) ui_set_visible(changelog_label, show_changelog);
            if (changelog_desc1) ui_set_visible(changelog_desc1, show_changelog);
            if (changelog_desc2) ui_set_visible(changelog_desc2, show_changelog);
            if (changelog_desc3) ui_set_visible(changelog_desc3, show_changelog);
            if (changelog_desc4) ui_set_visible(changelog_desc4, show_changelog);
            if (changelog_desc5) ui_set_visible(changelog_desc5, show_changelog);
        }

        // Yield to scheduler periodically
        if (counter % 100000 == 0) {
            __asm__ volatile("int $0x20");
        }
    }

    // Cleanup (in a real application, this would be called on exit)
    // ui_destroy_widget((ui_widget_t*)main_window);
}