#include "interfaces/user_mode.h"
#include "interfaces/subsystem_interfaces.h"

// Include subsystem implementations
// #include "integral_subsystems/workstation/workstation.c"
// #include "integral_subsystems/server_service/server_service.c"
// #include "integral_subsystems/security/security.c"
// #include "environment_subsystems/win32/win32.c"
// #include "environment_subsystems/posix/posix.c"
// #include "environment_subsystems/os2/os2.c"
// #include "compatibility_layers/windows9x/windows9x.c"
// #include "compatibility_layers/msdos/msdos.c"

// Subsystem interface pointers
workstation_interface_t* workstation = 0;
server_service_interface_t* server_service = 0;
security_interface_t* security = 0;
environment_interface_t* win32_env = 0;
environment_interface_t* posix_env = 0;
environment_interface_t* os2_env = 0;
compatibility_interface_t* windows9x_compat = 0;
compatibility_interface_t* msdos_compat = 0;

// User Mode initialization
void user_mode_init(void) {
    // Initialize integral subsystems
    workstation_init_impl();
    server_service_init_impl();
    security_init_impl();

    // Initialize environment subsystems
    win32_init_impl();
    posix_init_impl();
    os2_init_impl();

    // Initialize compatibility layers
    windows9x_init_impl();
    msdos_init_impl();
}

// User Mode API functions
void user_mode_shutdown(void) {
    // Shutdown all subsystems in reverse order
    msdos_shutdown_impl();
    windows9x_shutdown_impl();
    os2_shutdown_impl();
    posix_shutdown_impl();
    win32_shutdown_impl();
    security_shutdown_impl();
    server_service_shutdown_impl();
    workstation_shutdown_impl();
}

int user_mode_get_version(void) {
    return 0x010000; // Version 1.0.0
}