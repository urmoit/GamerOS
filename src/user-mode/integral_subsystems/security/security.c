#include "../../interfaces/subsystem_interfaces.h"
#include "../../../intf/string.h"

#define SECURITY_MAX_USERS 4
#define SECURITY_ACCESS_READ   0x01
#define SECURITY_ACCESS_WRITE  0x02
#define SECURITY_ACCESS_ADMIN  0x04

typedef struct {
    const char* username;
    const char* password;
    uint32_t user_id;
    uint32_t permissions;
} security_user_t;

static security_user_t security_users[SECURITY_MAX_USERS] = {
    {"admin", "gameros", 1, SECURITY_ACCESS_READ | SECURITY_ACCESS_WRITE | SECURITY_ACCESS_ADMIN},
    {"user", "user", 2, SECURITY_ACCESS_READ | SECURITY_ACCESS_WRITE},
    {"guest", "guest", 3, SECURITY_ACCESS_READ},
    {"service", "service", 4, SECURITY_ACCESS_READ | SECURITY_ACCESS_WRITE}
};

static uint32_t active_policy_id = 0;
static uint8_t security_ready = 0;

static uint32_t get_user_permissions(uint32_t user_id) {
    for (int i = 0; i < SECURITY_MAX_USERS; i++) {
        if (security_users[i].user_id == user_id) {
            return security_users[i].permissions;
        }
    }
    return 0;
}

// Function declarations
void security_init_impl(void);
void security_shutdown_impl(void);
int security_authenticate_user(const char* username, const char* password);
int security_check_access(uint32_t resource_id, uint32_t user_id, uint32_t access_type);
void security_set_security_policy(uint32_t policy_id);

// Security interface implementation
static security_interface_t security_interface = {
    .init = security_init_impl,
    .shutdown = security_shutdown_impl,
    .authenticate_user = security_authenticate_user,
    .check_access = security_check_access,
    .set_security_policy = security_set_security_policy
};

void security_init_impl(void) {
    // Initialize security components
    active_policy_id = 0;
    security_ready = 1;

    security = &security_interface;
}

void security_shutdown_impl(void) {
    // Shutdown security components
    security_ready = 0;
    security = 0;
}

int security_authenticate_user(const char* username, const char* password) {
    if (!security_ready || !username || !password) {
        return -1;
    }

    for (int i = 0; i < SECURITY_MAX_USERS; i++) {
        if (strcmp(username, security_users[i].username) == 0 &&
            strcmp(password, security_users[i].password) == 0) {
            return (int)security_users[i].user_id;
        }
    }
    return -1;
}

int security_check_access(uint32_t resource_id, uint32_t user_id, uint32_t access_type) {
    (void)resource_id;
    if (!security_ready) return -1;

    uint32_t perms = get_user_permissions(user_id);
    if (perms == 0) return -1;

    // Policy 1: strict policy, write requires admin capability.
    if (active_policy_id == 1 && (access_type & SECURITY_ACCESS_WRITE) &&
        (perms & SECURITY_ACCESS_ADMIN) == 0) {
        return -1;
    }

    if ((perms & access_type) == access_type) return 0;
    return -1;
}

void security_set_security_policy(uint32_t policy_id) {
    active_policy_id = policy_id;
}

// TODO: Replace plaintext password table with salted hash verification.
// TODO: Add authenticated session/token issuance instead of returning raw user IDs.
