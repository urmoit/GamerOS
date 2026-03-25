#ifndef FILESYSTEM_MANAGER_H
#define FILESYSTEM_MANAGER_H

#include <stdint.h>
#include "../object_manager/object_manager.h"

#define MAX_FILES 16
#define MAX_FILENAME_LEN 16
#define MAX_FILE_SIZE 256

typedef struct {
    char name[MAX_FILENAME_LEN];
    uint8_t data[MAX_FILE_SIZE];
    uint32_t size;
    uint8_t in_use;
    uint32_t disk_sector;
} file_t;

// File system operations
object_handle_t fs_open_file(const char* name, uint32_t flags);
int fs_close_file(object_handle_t file_handle);
int fs_read_file(object_handle_t file_handle, void* buffer, uint32_t size);
int fs_write_file(object_handle_t file_handle, const void* buffer, uint32_t size);
int fs_create_file(const char* name);
int fs_delete_file(const char* name);

// Directory operations (basic)
int fs_create_directory(const char* path);
int fs_remove_directory(const char* path);

// File system management
void filesystem_manager_init(void);

// File attributes
typedef struct {
    char name[MAX_FILENAME_LEN];
    uint32_t size;
    uint32_t attributes;
} file_info_t;

int fs_get_file_info(object_handle_t file_handle, file_info_t* info);

// TODO: Add file rename, copy, and move operations
// TODO: Define file permission and access control structures
// TODO: Add file timestamp support (creation, modification, access times)

#endif