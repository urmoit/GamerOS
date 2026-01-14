#ifndef SUBSYSTEM_INTERFACES_H
#define SUBSYSTEM_INTERFACES_H

#include "../../intf/stdint.h"

// Workstation Subsystem Interface
typedef struct {
    void (*init)(void);
    void (*shutdown)(void);
    void (*create_desktop)(void);
    void (*show_window)(uint32_t window_id);
    void (*hide_window)(uint32_t window_id);
    void (*move_window)(uint32_t window_id, int x, int y);
    void (*resize_window)(uint32_t window_id, uint32_t width, uint32_t height);
} workstation_interface_t;

// Server Service Subsystem Interface
typedef struct {
    void (*init)(void);
    void (*shutdown)(void);
    int (*start_service)(const char* service_name);
    int (*stop_service)(const char* service_name);
    int (*get_service_status)(const char* service_name);
} server_service_interface_t;

// Security Subsystem Interface
typedef struct {
    void (*init)(void);
    void (*shutdown)(void);
    int (*authenticate_user)(const char* username, const char* password);
    int (*check_access)(uint32_t resource_id, uint32_t user_id, uint32_t access_type);
    void (*set_security_policy)(uint32_t policy_id);
} security_interface_t;

// Environment Subsystem Base Interface
typedef struct {
    void (*init)(void);
    void (*shutdown)(void);
    int (*load_application)(const char* app_path);
    int (*execute_syscall)(uint32_t syscall_num, void* params);
    void* (*get_api_table)(void);
} environment_interface_t;

// Compatibility Layer Base Interface
typedef struct {
    void (*init)(void);
    void (*shutdown)(void);
    int (*enter_compatibility_mode)(void);
    int (*exit_compatibility_mode)(void);
    int (*translate_syscall)(uint32_t syscall_num, void* params);
} compatibility_interface_t;

// Global subsystem pointers
extern workstation_interface_t* workstation;
extern server_service_interface_t* server_service;
extern security_interface_t* security;
extern environment_interface_t* win32_env;
extern environment_interface_t* posix_env;
extern environment_interface_t* os2_env;
extern compatibility_interface_t* windows9x_compat;
extern compatibility_interface_t* msdos_compat;

#endif