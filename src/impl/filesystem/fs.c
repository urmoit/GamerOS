#include "../../intf/fs.h"
#include "../../intf/stdint.h"
#include "../../intf/string.h"

// Simple disk storage simulation (in reality, this would be on disk)
// Keep the in-kernel disk simulation small to avoid memory pressure in early boot.
#define DISK_SECTOR_SIZE 256
#define MAX_DISK_SECTORS 128
static uint8_t disk_storage[MAX_DISK_SECTORS * DISK_SECTOR_SIZE];

static file_t files[MAX_FILES];
static directory_t directories[MAX_DIRECTORIES];
static storage_device_info_t storage_devices[MAX_STORAGE_DEVICES];
static int storage_device_count = 0;
static uint32_t next_disk_sector = 0; // Track allocated disk sectors

static int paths_equal(const char* a, const char* b) {
    return strcmp(a, b) == 0;
}

static int is_prefix_path(const char* base, const char* path) {
    size_t base_len = strlen(base);
    if (base_len == 0) return 0;
    for (size_t i = 0; i < base_len; i++) {
        if (base[i] != path[i]) return 0;
    }
    if (base[base_len - 1] == '/') return 1;
    if (path[base_len] == 0 || path[base_len] == '/') return 1;
    return 0;
}

static int path_is_immediate_child(const char* parent, const char* path) {
    size_t parent_len = strlen(parent);
    if (!is_prefix_path(parent, path)) return 0;
    if (strlen(path) <= parent_len) return 0;
    const char* child = path + parent_len;
    if (*child == '/' && parent[parent_len - 1] != '/') child++;
    if (*child == 0) return 0;
    for (const char* p = child; *p; p++) {
        if (*p == '/') return 0;
    }
    return 1;
}

void fs_init() {
    // Initialize in-memory file table
    for (size_t i = 0; i < MAX_FILES; i++) {
        files[i].in_use = 0;
        files[i].size = 0;
        files[i].disk_sector = 0;
        memset(files[i].name, 0, MAX_FILENAME_LEN);
        memset(files[i].data, 0, MAX_FILE_SIZE);
    }
    for (size_t i = 0; i < MAX_DIRECTORIES; i++) {
        directories[i].in_use = 0;
        memset(directories[i].path, 0, MAX_FILENAME_LEN);
    }
    for (size_t i = 0; i < MAX_STORAGE_DEVICES; i++) {
        memset(storage_devices[i].name, 0, sizeof(storage_devices[i].name));
        storage_devices[i].type = STORAGE_HDD;
        storage_devices[i].capacity_mb = 0;
        storage_devices[i].online = 0;
    }
    storage_device_count = 0;

    // Initialize disk storage (in a real system, this would load from disk)
    memset(disk_storage, 0, sizeof(disk_storage));
    next_disk_sector = 1; // Reserve sector 0 for filesystem metadata

    fs_create_directory("C:/");
    fs_create_directory("C:/GamerOS");
    fs_create_directory("C:/GamerOS/System32");
    fs_create_directory("C:/Users");
    fs_create_directory("C:/Program Files");
    fs_create_directory("C:/Apps");

    // Simulated multi-disk support profiles.
    strncpy(storage_devices[0].name, "Disk0-HDD", sizeof(storage_devices[0].name) - 1);
    storage_devices[0].type = STORAGE_HDD;
    storage_devices[0].capacity_mb = 102400;
    storage_devices[0].online = 1;

    strncpy(storage_devices[1].name, "Disk1-SSD", sizeof(storage_devices[1].name) - 1);
    storage_devices[1].type = STORAGE_SSD;
    storage_devices[1].capacity_mb = 512000;
    storage_devices[1].online = 1;

    strncpy(storage_devices[2].name, "Disk2-NVMe", sizeof(storage_devices[2].name) - 1);
    storage_devices[2].type = STORAGE_NVME;
    storage_devices[2].capacity_mb = 1024000;
    storage_devices[2].online = 1;

    strncpy(storage_devices[3].name, "Disk3-USB", sizeof(storage_devices[3].name) - 1);
    storage_devices[3].type = STORAGE_USB;
    storage_devices[3].capacity_mb = 32768;
    storage_devices[3].online = 1;

    strncpy(storage_devices[4].name, "Disk4-CDROM", sizeof(storage_devices[4].name) - 1);
    storage_devices[4].type = STORAGE_CDROM;
    storage_devices[4].capacity_mb = 700;
    storage_devices[4].online = 1;

    strncpy(storage_devices[5].name, "Disk5-RAM", sizeof(storage_devices[5].name) - 1);
    storage_devices[5].type = STORAGE_RAMDISK;
    storage_devices[5].capacity_mb = 256;
    storage_devices[5].online = 1;

    storage_device_count = 6;
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

int fs_create_directory(const char* path) {
    if (!path || !path[0]) return 0;
    if (strlen(path) >= MAX_FILENAME_LEN) return 0;
    if (fs_directory_exists(path)) return 1;
    for (size_t i = 0; i < MAX_DIRECTORIES; i++) {
        if (!directories[i].in_use) {
            directories[i].in_use = 1;
            strncpy(directories[i].path, path, MAX_FILENAME_LEN - 1);
            directories[i].path[MAX_FILENAME_LEN - 1] = 0;
            return 1;
        }
    }
    return 0;
}

int fs_directory_exists(const char* path) {
    if (!path) return 0;
    for (size_t i = 0; i < MAX_DIRECTORIES; i++) {
        if (directories[i].in_use && paths_equal(directories[i].path, path)) {
            return 1;
        }
    }
    return 0;
}

int fs_file_exists(const char* path) {
    return fs_open_file(path) != 0;
}

uint32_t fs_get_file_size(const char* path) {
    file_t* file = fs_open_file(path);
    if (!file) return 0;
    return file->size;
}

int fs_list_entries(const char* directory, char out_names[][MAX_FILENAME_LEN], uint8_t out_is_dir[], int max_entries) {
    if (!directory || !out_names || !out_is_dir || max_entries <= 0) return 0;
    int count = 0;

    for (size_t i = 0; i < MAX_DIRECTORIES && count < max_entries; i++) {
        if (!directories[i].in_use) continue;
        if (!path_is_immediate_child(directory, directories[i].path)) continue;
        strncpy(out_names[count], directories[i].path, MAX_FILENAME_LEN - 1);
        out_names[count][MAX_FILENAME_LEN - 1] = 0;
        out_is_dir[count] = 1;
        count++;
    }

    for (size_t i = 0; i < MAX_FILES && count < max_entries; i++) {
        if (!files[i].in_use) continue;
        if (!path_is_immediate_child(directory, files[i].name)) continue;
        strncpy(out_names[count], files[i].name, MAX_FILENAME_LEN - 1);
        out_names[count][MAX_FILENAME_LEN - 1] = 0;
        out_is_dir[count] = 0;
        count++;
    }

    return count;
}

int fs_storage_get_device_count(void) {
    return storage_device_count;
}

int fs_storage_get_device(int index, storage_device_info_t* out) {
    if (!out || index < 0 || index >= storage_device_count) return 0;
    *out = storage_devices[index];
    return 1;
}

// Function to sync filesystem to disk (simulate persistence)
void fs_sync() {
    // In a real system, this would flush all cached data to disk
    // For now, our disk_storage array simulates persistent storage
    // Files are already written to disk_storage on write operations
}

// TODO: Implement directory creation, removal, and listing
// TODO: Add file permissions and access control
// TODO: Implement file system journaling for crash recovery
// TODO: Add support for file attributes (hidden, system, read-only)
