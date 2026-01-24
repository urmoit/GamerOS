#include "../intf/executive.h"
#include "object_manager/object_manager.h"
#include "filesystem_manager/filesystem_manager.h"
#include "io_manager/io_manager.h"

// Executive Layer initialization
void executive_init(void) {
    // Initialize Object Manager (fundamental service)
    object_manager_init();

    // Initialize Filesystem Manager
    filesystem_manager_init();

    // Initialize IO Manager
    io_manager_init();

    // TODO: Initialize other executive services when implemented
    // process_manager_init();
    // security_manager_init();
    // power_manager_init();
    // vmm_init();

    // TODO: Add error handling for initialization failures
    // TODO: Implement logging for executive layer startup
}