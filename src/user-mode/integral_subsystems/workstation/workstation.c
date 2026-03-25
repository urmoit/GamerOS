#include "../../interfaces/subsystem_interfaces.h"
#include "desktop_manager.h"
#include "window_manager.h"
#include "ui_framework.h"
#include "widget_system.h"

// Function declarations
void workstation_init_impl(void);
void workstation_shutdown_impl(void);
void workstation_create_desktop(void);
void workstation_show_window(uint32_t window_id);
void workstation_hide_window(uint32_t window_id);
void workstation_move_window(uint32_t window_id, int x, int y);
void workstation_resize_window(uint32_t window_id, uint32_t width, uint32_t height);

// Workstation interface implementation
static workstation_interface_t workstation_interface = {
    .init = workstation_init_impl,
    .shutdown = workstation_shutdown_impl,
    .create_desktop = workstation_create_desktop,
    .show_window = workstation_show_window,
    .hide_window = workstation_hide_window,
    .move_window = workstation_move_window,
    .resize_window = workstation_resize_window
};

void workstation_init_impl(void) {
    // Initialize all workstation components
    desktop_manager_init();
    window_manager_init();
    ui_framework_init();
    widget_system_init();

    // Set the global workstation interface
    workstation = &workstation_interface;
}

void workstation_shutdown_impl(void) {
    widget_system_shutdown();
    ui_framework_shutdown();
    window_manager_shutdown();
    desktop_manager_shutdown();
}

void workstation_create_desktop(void) {
    desktop_create();
}

void workstation_show_window(uint32_t window_id) {
    window_show(window_id);
}

void workstation_hide_window(uint32_t window_id) {
    window_hide(window_id);
}

void workstation_move_window(uint32_t window_id, int x, int y) {
    window_move(window_id, x, y);
}

void workstation_resize_window(uint32_t window_id, uint32_t width, uint32_t height) {
    window_resize(window_id, width, height);
}