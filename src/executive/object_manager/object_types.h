#ifndef OBJECT_TYPES_H
#define OBJECT_TYPES_H

#include "../../intf/stdint.h"

// Base object structure
typedef struct object {
    object_type_t type;
    uint32_t reference_count;
    void* context;
    void* security_descriptor;
    struct object* next;
} object_t;

// Process object
typedef struct {
    object_t base;
    uint32_t pid;
    uint32_t parent_pid;
    uint32_t state;
    void* address_space;
} process_object_t;

// Thread object
typedef struct {
    object_t base;
    uint32_t tid;
    uint32_t pid;
    uint32_t state;
    void* stack;
} thread_object_t;

// File object
typedef struct {
    object_t base;
    char* path;
    uint32_t flags;
    uint32_t position;
    void* filesystem_context;
} file_object_t;

// Device object
typedef struct {
    object_t base;
    char* name;
    uint32_t type;
    void* driver_context;
} device_object_t;

// Memory segment object
typedef struct {
    object_t base;
    void* virtual_address;
    uint32_t size;
    uint32_t protection;
} memory_segment_object_t;

// Synchronization objects
typedef struct {
    object_t base;
    uint32_t count;
    uint32_t max_count;
} semaphore_object_t;

typedef struct {
    object_t base;
    uint32_t owner_tid;
    uint32_t lock_count;
} mutex_object_t;

typedef struct {
    object_t base;
    uint8_t signaled;
} event_object_t;

// IPC objects
typedef struct {
    object_t base;
    char* name;
    void* buffer;
    uint32_t size;
} pipe_object_t;

typedef struct {
    object_t base;
    char* name;
    void* address;
    uint32_t size;
} shared_memory_object_t;

#endif