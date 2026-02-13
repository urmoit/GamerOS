#include "../intf/executive.h"
#include "object_manager/object_manager.h"
#include "filesystem_manager/filesystem_manager.h"
#include "io_manager/io_manager.h"
#include "gdi/gdi.h"

extern device_driver_t keyboard_driver;
extern device_driver_t mouse_driver;

// Executive Layer initialization
void executive_init(void) {
    // Initialize Object Manager (fundamental service)
    object_manager_init();

    // Initialize Filesystem Manager
    filesystem_manager_init();

    // Initialize IO Manager
    io_manager_init();

    // Initialize graphics service
    gdi_init();

    // Register baseline input drivers
    io_register_driver(&keyboard_driver);
    io_register_driver(&mouse_driver);

    // Create baseline filesystem objects so executive consumers can open them.
    fs_create_file("README.TXT");
    fs_create_file("NOTEPAD.TXT");

    // TODO: Add error handling for initialization failures
    // TODO: Implement logging for executive layer startup
}
