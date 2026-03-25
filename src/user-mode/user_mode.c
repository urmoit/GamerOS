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

#define USER_MODE_MAX_PROCESSES 32
#define USER_MODE_SEC_SYSTEM    3
#define USER_MODE_SEC_SERVICE   2
#define USER_MODE_SEC_USER      1

typedef struct {
    uint32_t pid;
    uint32_t owner_uid;
    uint32_t security_level;
    uintptr_t isolation_tag;
    uint8_t active;
} user_process_t;

static user_process_t process_table[USER_MODE_MAX_PROCESSES];
static uint32_t next_pid = 1;
static uint8_t user_mode_security_ready = 0;

static void user_mode_reset_process_table(void) {
    for (int i = 0; i < USER_MODE_MAX_PROCESSES; i++) {
        process_table[i].pid = 0;
        process_table[i].owner_uid = 0;
        process_table[i].security_level = 0;
        process_table[i].isolation_tag = 0;
        process_table[i].active = 0;
    }
    next_pid = 1;
}

static int user_mode_create_isolated_process(uint32_t owner_uid, uint32_t security_level) {
    for (int i = 0; i < USER_MODE_MAX_PROCESSES; i++) {
        if (!process_table[i].active) {
            uint32_t pid = next_pid++;
            process_table[i].pid = pid;
            process_table[i].owner_uid = owner_uid;
            process_table[i].security_level = security_level;
            process_table[i].isolation_tag = ((uintptr_t)pid << 20) ^ ((uintptr_t)owner_uid << 8) ^ security_level;
            process_table[i].active = 1;
            return (int)pid;
        }
    }
    return -1;
}

static int user_mode_validate_process_access(uint32_t caller_pid, uint32_t target_pid) {
    user_process_t* caller = 0;
    user_process_t* target = 0;
    for (int i = 0; i < USER_MODE_MAX_PROCESSES; i++) {
        if (process_table[i].active && process_table[i].pid == caller_pid) caller = &process_table[i];
        if (process_table[i].active && process_table[i].pid == target_pid) target = &process_table[i];
    }
    if (!caller || !target) return -1;
    if (caller->pid == target->pid) return 0;
    if (caller->security_level >= USER_MODE_SEC_SYSTEM) return 0;
    if (caller->owner_uid == target->owner_uid && caller->security_level >= target->security_level) return 0;
    return -1;
}

// Subsystem lifecycle entry points
extern void workstation_init_impl(void);
extern void workstation_shutdown_impl(void);
extern void server_service_init_impl(void);
extern void server_service_shutdown_impl(void);
extern void security_init_impl(void);
extern void security_shutdown_impl(void);
extern void win32_init_impl(void);
extern void win32_shutdown_impl(void);
extern void posix_init_impl(void);
extern void posix_shutdown_impl(void);
extern void os2_init_impl(void);
extern void os2_shutdown_impl(void);
extern void windows9x_init_impl(void);
extern void windows9x_shutdown_impl(void);
extern void msdos_init_impl(void);
extern void msdos_shutdown_impl(void);

// User Mode initialization
void user_mode_init(void) {
    user_mode_reset_process_table();

    // Initialize integral subsystems
    workstation_init_impl();
    server_service_init_impl();
    security_init_impl();
    user_mode_security_ready = (security && security->authenticate_user && security->check_access) ? 1 : 0;

    uint32_t admin_uid = 0;
    if (user_mode_security_ready) {
        int auth_uid = security->authenticate_user("admin", "gameros");
        if (auth_uid >= 0) {
            admin_uid = (uint32_t)auth_uid;
        }
    }

    // Establish isolated process contexts for core user-mode subsystems.
    int ws_pid = user_mode_create_isolated_process(admin_uid, USER_MODE_SEC_SYSTEM);
    int ss_pid = user_mode_create_isolated_process(admin_uid, USER_MODE_SEC_SERVICE);
    int sec_pid = user_mode_create_isolated_process(admin_uid, USER_MODE_SEC_SYSTEM);
    (void)ws_pid;
    (void)ss_pid;
    (void)sec_pid;

    // Initialize environment subsystems
    int win32_pid = user_mode_create_isolated_process(admin_uid, USER_MODE_SEC_USER);
    int posix_pid = user_mode_create_isolated_process(admin_uid, USER_MODE_SEC_USER);
    int os2_pid = user_mode_create_isolated_process(admin_uid, USER_MODE_SEC_USER);
    if (win32_pid >= 0) win32_init_impl();
    if (posix_pid >= 0) posix_init_impl();
    if (os2_pid >= 0) os2_init_impl();

    // Initialize compatibility layers
    int win9x_pid = user_mode_create_isolated_process(admin_uid, USER_MODE_SEC_USER);
    int dos_pid = user_mode_create_isolated_process(admin_uid, USER_MODE_SEC_USER);
    if (win9x_pid >= 0) windows9x_init_impl();
    if (dos_pid >= 0) msdos_init_impl();

    // Basic isolation validation between subsystem processes.
    if (ws_pid >= 0 && win32_pid >= 0) {
        (void)user_mode_validate_process_access((uint32_t)win32_pid, (uint32_t)ws_pid);
    }
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

// TODO: Add user mode system call interface
// TODO: Implement user mode memory management
// TODO: Add support for loading and executing user applications
