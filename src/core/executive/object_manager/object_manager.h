#ifndef OBJECT_MANAGER_H
#define OBJECT_MANAGER_H

#include "../../intf/stdint.h"

// Object types
typedef enum {
    OBJECT_TYPE_PROCESS,
    OBJECT_TYPE_THREAD,
    OBJECT_TYPE_FILE,
    OBJECT_TYPE_DEVICE,
    OBJECT_TYPE_MEMORY_SEGMENT,
    OBJECT_TYPE_SEMAPHORE,
    OBJECT_TYPE_MUTEX,
    OBJECT_TYPE_EVENT,
    OBJECT_TYPE_PIPE,
    OBJECT_TYPE_SHARED_MEMORY,
    OBJECT_TYPE_MAX
} object_type_t;

// Object handle
typedef uint32_t object_handle_t;

// Object attributes
typedef struct {
    object_type_t type;
    uint32_t flags;
    void* security_descriptor;
    void* context;
} object_attributes_t;

// Invalid handle value
#define INVALID_HANDLE 0xFFFFFFFF

// Object Manager functions
object_handle_t object_create(object_type_t type, object_attributes_t* attributes);
int object_destroy(object_handle_t handle);
int object_reference(object_handle_t handle);
int object_dereference(object_handle_t handle);
void* object_get_context(object_handle_t handle);
object_type_t object_get_type(object_handle_t handle);

// Initialization
void object_manager_init(void);

// TODO: Add object_wait and object_signal functions for synchronization
// TODO: Implement object duplication and inheritance
// TODO: Add object attribute querying and modification

#endif