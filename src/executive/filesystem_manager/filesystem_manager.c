#include "filesystem_manager.h"
#include "../../intf/string.h"
#include "../../intf/memory.h"

#ifndef NULL
#define NULL ((void*)0)
#endif

// Simple disk storage simulation (in reality, this would be on disk)
#define DISK_SECTOR_SIZE 512
#define MAX_DISK_SECTORS 1024
static uint8_t disk_storage[MAX_DISK_SECTORS * DISK_SECTOR_SIZE];

static file_t files[MAX_FILES];
static uint32_t next_disk_sector = 0; // Track allocated disk sectors

void filesystem_manager_init() {
    // Initialize in-memory file table
    for (size_t i = 0; i < MAX_FILES; i++) {
        files[i].in_use = 0;
        files[i].size = 0;
        files[i].disk_sector = 0;
        memset(files[i].name, 0, MAX_FILENAME_LEN);
        memset(files[i].data, 0, MAX_FILE_SIZE);
    }

    // Initialize disk storage (in a real system, this would load from disk)
    memset(disk_storage, 0, sizeof(disk_storage));
    next_disk_sector = 1; // Reserve sector 0 for filesystem metadata
}

static file_t* find_file_by_name(const char* name) {
    for (size_t i = 0; i < MAX_FILES; i++) {
        if (files[i].in_use && strcmp(files[i].name, name) == 0) {
            return &files[i];
        }
    }
    return NULL;
}

static file_t* allocate_file_slot() {
    for (size_t i = 0; i < MAX_FILES; i++) {
        if (!files[i].in_use) {
            return &files[i];
        }
    }
    return NULL;
}

object_handle_t fs_open_file(const char* name, uint32_t flags) {
    (void)flags;  // For now, ignore flags
    // TODO: Implement file open flags (read-only, write-only, append, create, etc.)

    file_t* file = find_file_by_name(name);
    if (!file) {
        return INVALID_HANDLE;
    }

    // Create file object
    object_attributes_t attrs = {
        .type = OBJECT_TYPE_FILE,
        .context = file
    };
    return object_create(OBJECT_TYPE_FILE, &attrs);
}

int fs_close_file(object_handle_t file_handle) {
    if (object_get_type(file_handle) != OBJECT_TYPE_FILE) {
        return -1;
    }
    return object_destroy(file_handle);
}

int fs_read_file(object_handle_t file_handle, void* buffer, uint32_t size) {
    if (object_get_type(file_handle) != OBJECT_TYPE_FILE || !buffer) {
        return -1;
    }

    file_t* file = (file_t*)object_get_context(file_handle);
    if (!file || !file->in_use) {
        return -1;
    }

    // Read from disk storage first (simulate persistence), then fall back to memory cache
    if (file->disk_sector < MAX_DISK_SECTORS) {
        uint32_t bytes_to_read = (size < file->size) ? size : file->size;
        if (bytes_to_read <= DISK_SECTOR_SIZE) {
            memcpy(buffer, &disk_storage[file->disk_sector * DISK_SECTOR_SIZE], bytes_to_read);
            // Zero out remaining buffer space if requested size > file size
            if (size > bytes_to_read) {
                memset((uint8_t*)buffer + bytes_to_read, 0, size - bytes_to_read);
            }
            return (int)bytes_to_read;
        }
    }

    // Fall back to memory cache
    uint32_t bytes_to_read = (size < file->size) ? size : file->size;
    memcpy(buffer, file->data, bytes_to_read);
    // Zero out remaining buffer space if requested size > file size
    if (size > bytes_to_read) {
        memset((uint8_t*)buffer + bytes_to_read, 0, size - bytes_to_read);
    }
    return (int)bytes_to_read;
}

int fs_write_file(object_handle_t file_handle, const void* buffer, uint32_t size) {
    if (object_get_type(file_handle) != OBJECT_TYPE_FILE || !buffer || size >= MAX_FILE_SIZE) {
        return -1;
    }

    file_t* file = (file_t*)object_get_context(file_handle);
    if (!file || !file->in_use) {
        return -1;
    }

    // Write to both memory cache and disk storage
    memcpy(file->data, buffer, size);
    file->size = size;

    // Write to disk sector (simulate persistence)
    if (file->disk_sector < MAX_DISK_SECTORS && size <= DISK_SECTOR_SIZE) {
        memcpy(&disk_storage[file->disk_sector * DISK_SECTOR_SIZE], buffer, size);
    }

    return (int)size;
}

int fs_create_file(const char* name) {
    if (!name) return -1;

    // Check if file already exists
    if (find_file_by_name(name)) {
        return -2; // File already exists
    }

    file_t* file = allocate_file_slot();
    if (!file) {
        return -3; // No space left
    }

    strncpy(file->name, name, MAX_FILENAME_LEN - 1);
    file->name[MAX_FILENAME_LEN - 1] = '\0'; // Ensure null termination
    file->in_use = 1;
    file->size = 0;
    file->disk_sector = next_disk_sector++;

    // Initialize disk sector
    if (file->disk_sector < MAX_DISK_SECTORS) {
        memset(&disk_storage[file->disk_sector * DISK_SECTOR_SIZE], 0, DISK_SECTOR_SIZE);
    }

    return 0;
}

// TODO: Add file locking mechanism for concurrent access
// TODO: Implement file seeking operations (fs_seek_file)
// TODO: Add support for file attributes (hidden, system, read-only)
// TODO: Implement file system journaling for crash recovery

int fs_delete_file(const char* name) {
    file_t* file = find_file_by_name(name);
    if (!file) {
        return -1; // File not found
    }

    file->in_use = 0;
    file->size = 0;
    memset(file->name, 0, MAX_FILENAME_LEN);
    memset(file->data, 0, MAX_FILE_SIZE);

    return 0;
}

int fs_create_directory(const char* path) {
    (void)path;
    // Basic implementation - directories not fully supported yet
    // TODO: Implement directory creation with path parsing, validation, and parent directory checks
    return -1;
}

int fs_remove_directory(const char* path) {
    (void)path;
    // Basic implementation - directories not fully supported yet
    // TODO: Implement directory removal with checks for empty directories and recursive options
    return -1;
}

int fs_get_file_info(object_handle_t file_handle, file_info_t* info) {
    if (object_get_type(file_handle) != OBJECT_TYPE_FILE || !info) {
        return -1;
    }

    file_t* file = (file_t*)object_get_context(file_handle);
    if (!file || !file->in_use) {
        return -1;
    }

    strncpy(info->name, file->name, MAX_FILENAME_LEN);
    info->size = file->size;
    info->attributes = 0;  // Basic attributes

    return 0;
}