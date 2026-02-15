#include "../intf/executive.h"
#include "object_manager/object_manager.h"
#include "filesystem_manager/filesystem_manager.h"
#include "io_manager/io_manager.h"
#include "gdi/gdi.h"

extern device_driver_t keyboard_driver;
extern device_driver_t mouse_driver;

static int executive_ready = 0;
static int executive_last_error = EXECUTIVE_ERR_NONE;

int executive_is_ready(void) {
    return executive_ready;
}

int executive_get_last_error(void) {
    return executive_last_error;
}

// Executive Layer initialization
void executive_init(void) {
    executive_ready = 0;
    executive_last_error = EXECUTIVE_ERR_NONE;

    // Initialize Object Manager (fundamental service)
    object_manager_init();

    // Initialize Filesystem Manager
    filesystem_manager_init();

    // Initialize IO Manager
    io_manager_init();

    // Initialize graphics service
    gdi_init();
    if (!gdi_is_ready()) {
        executive_last_error = EXECUTIVE_ERR_GDI_INIT;
        return;
    }

    // Register baseline input drivers
    if (io_register_driver(&keyboard_driver) != 0 ||
        io_register_driver(&mouse_driver) != 0) {
        executive_last_error = EXECUTIVE_ERR_DRIVER_REGISTER;
        return;
    }

    // Create baseline filesystem objects so executive consumers can open them.
    if (fs_create_file("README.TXT") != 0 ||
        fs_create_file("NOTEPAD.TXT") != 0) {
        executive_last_error = EXECUTIVE_ERR_FS_BOOTSTRAP;
        return;
    }

    executive_ready = 1;
    // TODO: Implement logging for executive layer startup
}
