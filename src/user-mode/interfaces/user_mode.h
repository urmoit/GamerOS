#ifndef USER_MODE_H
#define USER_MODE_H

#include "../../intf/stdint.h"

// User Mode initialization
void user_mode_init(void);

// Subsystem initialization functions
void workstation_init(void);
void server_service_init(void);
void security_init(void);

// Environment subsystem initialization
void win32_init(void);
void posix_init(void);
void os2_init(void);

// Compatibility layer initialization
void windows9x_init(void);
void msdos_init(void);

// User Mode API functions
void user_mode_shutdown(void);
int user_mode_get_version(void);

// TODO: Add user mode process creation and management APIs
// TODO: Implement user mode inter-process communication
// TODO: Add user mode file system access functions
// TODO: Implement user mode networking APIs

#endif