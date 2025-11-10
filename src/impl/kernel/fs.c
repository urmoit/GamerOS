#include "../../intf/fs.h"
#include "../../intf/stdint.h"
#include "../../intf/string.h"

static file_t files[MAX_FILES];

void fs_init() {
    for (int i = 0; i < MAX_FILES; i++) {
        files[i].in_use = 0;
        files[i].size = 0;
        memset(files[i].name, 0, MAX_FILENAME_LEN);
        memset(files[i].data, 0, MAX_FILE_SIZE);
    }
}

file_t* fs_create_file(const char* name) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (!files[i].in_use) {
            strncpy(files[i].name, name, MAX_FILENAME_LEN - 1);
            files[i].name[MAX_FILENAME_LEN - 1] = '\0'; // Ensure null termination
            files[i].in_use = 1;
            files[i].size = 0;
            return &files[i];
        }
    }
    return 0; // No space left
}

file_t* fs_open_file(const char* name) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].in_use && strcmp(files[i].name, name) == 0) {
            return &files[i];
        }
    }
    return 0; // File not found
}

void fs_write_file(file_t* file, const uint8_t* data, uint32_t size) {
    if (!file || !file->in_use || size >= MAX_FILE_SIZE) return;
    memcpy(file->data, data, size);
    file->size = size;
}

void fs_read_file(file_t* file, uint8_t* buffer, uint32_t size) {
    if (!file || !file->in_use || !buffer || size == 0) return;
    uint32_t bytes_to_read = (size < file->size) ? size : file->size;
    memcpy(buffer, file->data, bytes_to_read);
}