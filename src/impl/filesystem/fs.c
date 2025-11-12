#include "../../intf/fs.h"
#include "../../intf/stdint.h"
#include "../../intf/string.h"

// Simple disk storage simulation (in reality, this would be on disk)
#define DISK_SECTOR_SIZE 512
#define MAX_DISK_SECTORS 1024
static uint8_t disk_storage[MAX_DISK_SECTORS * DISK_SECTOR_SIZE];

static file_t files[MAX_FILES];
static uint32_t next_disk_sector = 0; // Track allocated disk sectors

void fs_init() {
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

file_t* fs_create_file(const char* name) {
    if (!name) return 0; // NULL check
    for (size_t i = 0; i < MAX_FILES; i++) {
        if (!files[i].in_use) {
            // Check if file already exists
            if (fs_open_file(name)) {
                return 0; // File already exists
            }

            strncpy(files[i].name, name, MAX_FILENAME_LEN - 1);
            files[i].name[MAX_FILENAME_LEN - 1] = '\0'; // Ensure null termination
            files[i].in_use = 1;
            files[i].size = 0;
            files[i].disk_sector = next_disk_sector++;

            // Initialize disk sector
            if (files[i].disk_sector < MAX_DISK_SECTORS) {
                memset(&disk_storage[files[i].disk_sector * DISK_SECTOR_SIZE], 0, DISK_SECTOR_SIZE);
            }

            return &files[i];
        }
    }
    return 0; // No space left
}

file_t* fs_open_file(const char* name) {
    if (!name) return 0; // NULL check
    for (size_t i = 0; i < MAX_FILES; i++) {
        if (files[i].in_use && strcmp(files[i].name, name) == 0) {
            return &files[i];
        }
    }
    return 0; // File not found
}

void fs_write_file(file_t* file, const uint8_t* data, uint32_t size) {
    if (!file || !file->in_use || !data || size >= MAX_FILE_SIZE) return;

    // Write to both memory cache and disk storage
    memcpy(file->data, data, size);
    file->size = size;

    // Write to disk sector (simulate persistence)
    if (file->disk_sector < MAX_DISK_SECTORS && size <= DISK_SECTOR_SIZE) {
        memcpy(&disk_storage[file->disk_sector * DISK_SECTOR_SIZE], data, size);
    }
}

void fs_read_file(file_t* file, uint8_t* buffer, uint32_t size) {
    if (!file || !file->in_use || !buffer || size == 0) return;

    // Read from disk storage first (simulate persistence), then fall back to memory cache
    if (file->disk_sector < MAX_DISK_SECTORS) {
        uint32_t bytes_to_read = (size < file->size) ? size : file->size;
        if (bytes_to_read <= DISK_SECTOR_SIZE) {
            memcpy(buffer, &disk_storage[file->disk_sector * DISK_SECTOR_SIZE], bytes_to_read);
            // Zero out remaining buffer space if requested size > file size
            if (size > bytes_to_read) {
                memset(buffer + bytes_to_read, 0, size - bytes_to_read);
            }
            return;
        }
    }

    // Fall back to memory cache
    uint32_t bytes_to_read = (size < file->size) ? size : file->size;
    memcpy(buffer, file->data, bytes_to_read);
    // Zero out remaining buffer space if requested size > file size
    if (size > bytes_to_read) {
        memset(buffer + bytes_to_read, 0, size - bytes_to_read);
    }
}

// Function to sync filesystem to disk (simulate persistence)
void fs_sync() {
    // In a real system, this would flush all cached data to disk
    // For now, our disk_storage array simulates persistent storage
    // Files are already written to disk_storage on write operations
}