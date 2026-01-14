#include "object_manager.h"
#include "object_types.h"
#include "../../intf/memory.h"  // For kmalloc/kfree
#include "../../intf/string.h"  // For memset

#ifndef NULL
#define NULL ((void*)0)
#endif

#define MAX_OBJECTS 1024

static object_t* object_table[MAX_OBJECTS];
static uint32_t next_handle = 1;  // Start from 1, 0 is invalid

void object_manager_init(void) {
    memset(object_table, 0, sizeof(object_table));
    next_handle = 1;
}

static object_handle_t allocate_handle(void) {
    if (next_handle >= MAX_OBJECTS) {
        // Handle wraparound or find free slot
        for (uint32_t i = 1; i < MAX_OBJECTS; i++) {
            if (object_table[i] == NULL) {
                return i;
            }
        }
        return INVALID_HANDLE;  // No free handles
    }
    return next_handle++;
}

static object_t* get_object(object_handle_t handle) {
    if (handle >= MAX_OBJECTS || handle == INVALID_HANDLE) {
        return NULL;
    }
    return object_table[handle];
}

object_handle_t object_create(object_type_t type, object_attributes_t* attributes) {
    // Allocate object based on type
    object_t* obj = NULL;
    size_t size = 0;

    switch (type) {
        case OBJECT_TYPE_PROCESS:
            size = sizeof(process_object_t);
            break;
        case OBJECT_TYPE_THREAD:
            size = sizeof(thread_object_t);
            break;
        case OBJECT_TYPE_FILE:
            size = sizeof(file_object_t);
            break;
        case OBJECT_TYPE_DEVICE:
            size = sizeof(device_object_t);
            break;
        case OBJECT_TYPE_MEMORY_SEGMENT:
            size = sizeof(memory_segment_object_t);
            break;
        case OBJECT_TYPE_SEMAPHORE:
            size = sizeof(semaphore_object_t);
            break;
        case OBJECT_TYPE_MUTEX:
            size = sizeof(mutex_object_t);
            break;
        case OBJECT_TYPE_EVENT:
            size = sizeof(event_object_t);
            break;
        case OBJECT_TYPE_PIPE:
            size = sizeof(pipe_object_t);
            break;
        case OBJECT_TYPE_SHARED_MEMORY:
            size = sizeof(shared_memory_object_t);
            break;
        default:
            return INVALID_HANDLE;
    }

    obj = (object_t*)kmalloc(size);
    if (!obj) {
        return INVALID_HANDLE;
    }

    // Initialize object
    memset(obj, 0, size);
    obj->type = type;
    obj->reference_count = 1;
    obj->context = attributes ? attributes->context : NULL;
    obj->security_descriptor = attributes ? attributes->security_descriptor : NULL;

    // Allocate handle
    object_handle_t handle = allocate_handle();
    if (handle == INVALID_HANDLE) {
        kfree(obj);
        return INVALID_HANDLE;
    }

    object_table[handle] = obj;
    return handle;
}

int object_destroy(object_handle_t handle) {
    object_t* obj = get_object(handle);
    if (!obj) {
        return -1;  // Invalid handle
    }

    if (obj->reference_count > 1) {
        return -2;  // Still referenced
    }

    // Free object-specific resources if needed
    // For now, just free the object
    kfree(obj);
    object_table[handle] = NULL;

    return 0;
}

int object_reference(object_handle_t handle) {
    object_t* obj = get_object(handle);
    if (!obj) {
        return -1;
    }

    obj->reference_count++;
    return 0;
}

int object_dereference(object_handle_t handle) {
    object_t* obj = get_object(handle);
    if (!obj) {
        return -1;
    }

    if (obj->reference_count == 0) {
        return -2;  // Already zero
    }

    obj->reference_count--;
    if (obj->reference_count == 0) {
        // Auto-destroy when reference count reaches zero
        return object_destroy(handle);
    }

    return 0;
}

void* object_get_context(object_handle_t handle) {
    object_t* obj = get_object(handle);
    return obj ? obj->context : NULL;
}

object_type_t object_get_type(object_handle_t handle) {
    object_t* obj = get_object(handle);
    return obj ? obj->type : OBJECT_TYPE_MAX;
}