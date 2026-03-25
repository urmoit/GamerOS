#include "../../intf/fs.h"
#include "../../intf/ata_pio.h"
#include "../../intf/stdint.h"
#include "../../intf/string.h"

#define DISK_SECTOR_SIZE ATA_SECTOR_SIZE
#define MAX_DISK_SECTORS 4096
#define FS_FILE_RESERVED_SECTORS ((MAX_FILE_SIZE + DISK_SECTOR_SIZE - 1) / DISK_SECTOR_SIZE)

#define FS_SUPERBLOCK_SECTOR 0
#define FS_FILE_TABLE_START 1
#define FS_FILE_TABLE_SECTORS 4
#define FS_DIR_TABLE_START (FS_FILE_TABLE_START + FS_FILE_TABLE_SECTORS)
#define FS_DIR_TABLE_SECTORS 3
#define FS_DATA_START_SECTOR (FS_DIR_TABLE_START + FS_DIR_TABLE_SECTORS)

#define FS_MAGIC "GFS1PIO"
#define FS_VERSION 1

// Fallback RAM store if ATA is unavailable.
static uint8_t disk_storage[MAX_DISK_SECTORS * DISK_SECTOR_SIZE];
static uint8_t g_fs_ata_enabled = 0;

typedef struct {
    char magic[8];
    uint32_t version;
    uint32_t next_disk_sector;
    uint32_t reserved0;
    uint8_t padding[DISK_SECTOR_SIZE - 20];
} fs_superblock_t;

typedef struct {
    char name[MAX_FILENAME_LEN];
    uint32_t size;
    uint32_t disk_sector;
    uint8_t in_use;
    uint8_t reserved[15];
} fs_disk_file_record_t;

typedef struct {
    char path[MAX_FILENAME_LEN];
    uint8_t in_use;
    uint8_t reserved[7];
} fs_disk_dir_record_t;

static file_t files[MAX_FILES];
static directory_t directories[MAX_DIRECTORIES];
static storage_device_info_t storage_devices[MAX_STORAGE_DEVICES];
static int storage_device_count = 0;
static uint32_t next_disk_sector = 0; // Track allocated disk sectors

static int fs_disk_read_sector(uint32_t lba, uint8_t* out) {
    if (!out || lba >= MAX_DISK_SECTORS) return 0;
    if (g_fs_ata_enabled) return ata_pio_read_sector(lba, out);
    memcpy(out, &disk_storage[lba * DISK_SECTOR_SIZE], DISK_SECTOR_SIZE);
    return 1;
}

static int fs_disk_write_sector(uint32_t lba, const uint8_t* in) {
    if (!in || lba >= MAX_DISK_SECTORS) return 0;
    if (g_fs_ata_enabled) return ata_pio_write_sector(lba, in);
    memcpy(&disk_storage[lba * DISK_SECTOR_SIZE], in, DISK_SECTOR_SIZE);
    return 1;
}

static void fs_reset_tables(void) {
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
    next_disk_sector = FS_DATA_START_SECTOR;
}

static void fs_save_metadata(void) {
    fs_superblock_t sb;
    memset(&sb, 0, sizeof(sb));
    memcpy(sb.magic, FS_MAGIC, sizeof(sb.magic));
    sb.version = FS_VERSION;
    sb.next_disk_sector = next_disk_sector;

    (void)fs_disk_write_sector(FS_SUPERBLOCK_SECTOR, (const uint8_t*)&sb);

    uint8_t sector[DISK_SECTOR_SIZE];
    for (uint32_t si = 0; si < FS_FILE_TABLE_SECTORS; si++) {
        memset(sector, 0, sizeof(sector));
        size_t recs_per_sector = DISK_SECTOR_SIZE / sizeof(fs_disk_file_record_t);
        for (size_t r = 0; r < recs_per_sector; r++) {
            size_t idx = si * recs_per_sector + r;
            if (idx >= MAX_FILES) break;
            fs_disk_file_record_t rec;
            memset(&rec, 0, sizeof(rec));
            strncpy(rec.name, files[idx].name, MAX_FILENAME_LEN - 1);
            rec.size = files[idx].size;
            rec.disk_sector = files[idx].disk_sector;
            rec.in_use = files[idx].in_use;
            memcpy(&sector[r * sizeof(fs_disk_file_record_t)], &rec, sizeof(rec));
        }
        (void)fs_disk_write_sector(FS_FILE_TABLE_START + si, sector);
    }

    for (uint32_t si = 0; si < FS_DIR_TABLE_SECTORS; si++) {
        memset(sector, 0, sizeof(sector));
        size_t recs_per_sector = DISK_SECTOR_SIZE / sizeof(fs_disk_dir_record_t);
        for (size_t r = 0; r < recs_per_sector; r++) {
            size_t idx = si * recs_per_sector + r;
            if (idx >= MAX_DIRECTORIES) break;
            fs_disk_dir_record_t rec;
            memset(&rec, 0, sizeof(rec));
            strncpy(rec.path, directories[idx].path, MAX_FILENAME_LEN - 1);
            rec.in_use = directories[idx].in_use;
            memcpy(&sector[r * sizeof(fs_disk_dir_record_t)], &rec, sizeof(rec));
        }
        (void)fs_disk_write_sector(FS_DIR_TABLE_START + si, sector);
    }
}

static int fs_load_metadata(void) {
    fs_superblock_t sb;
    if (!fs_disk_read_sector(FS_SUPERBLOCK_SECTOR, (uint8_t*)&sb)) return 0;
    if (strncmp(sb.magic, FS_MAGIC, 7) != 0) return 0;
    if (sb.version != FS_VERSION) return 0;

    fs_reset_tables();
    next_disk_sector = sb.next_disk_sector;
    if (next_disk_sector < FS_DATA_START_SECTOR) next_disk_sector = FS_DATA_START_SECTOR;
    if (next_disk_sector >= MAX_DISK_SECTORS) next_disk_sector = FS_DATA_START_SECTOR;

    uint8_t sector[DISK_SECTOR_SIZE];
    for (uint32_t si = 0; si < FS_FILE_TABLE_SECTORS; si++) {
        if (!fs_disk_read_sector(FS_FILE_TABLE_START + si, sector)) return 0;
        size_t recs_per_sector = DISK_SECTOR_SIZE / sizeof(fs_disk_file_record_t);
        for (size_t r = 0; r < recs_per_sector; r++) {
            size_t idx = si * recs_per_sector + r;
            if (idx >= MAX_FILES) break;
            fs_disk_file_record_t rec;
            memcpy(&rec, &sector[r * sizeof(fs_disk_file_record_t)], sizeof(rec));
            strncpy(files[idx].name, rec.name, MAX_FILENAME_LEN - 1);
            files[idx].name[MAX_FILENAME_LEN - 1] = 0;
            files[idx].size = rec.size;
            files[idx].disk_sector = rec.disk_sector;
            files[idx].in_use = rec.in_use;
        }
    }

    for (uint32_t si = 0; si < FS_DIR_TABLE_SECTORS; si++) {
        if (!fs_disk_read_sector(FS_DIR_TABLE_START + si, sector)) return 0;
        size_t recs_per_sector = DISK_SECTOR_SIZE / sizeof(fs_disk_dir_record_t);
        for (size_t r = 0; r < recs_per_sector; r++) {
            size_t idx = si * recs_per_sector + r;
            if (idx >= MAX_DIRECTORIES) break;
            fs_disk_dir_record_t rec;
            memcpy(&rec, &sector[r * sizeof(fs_disk_dir_record_t)], sizeof(rec));
            strncpy(directories[idx].path, rec.path, MAX_FILENAME_LEN - 1);
            directories[idx].path[MAX_FILENAME_LEN - 1] = 0;
            directories[idx].in_use = rec.in_use;
        }
    }
    return 1;
}

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
    fs_reset_tables();
    for (size_t i = 0; i < MAX_STORAGE_DEVICES; i++) {
        memset(storage_devices[i].name, 0, sizeof(storage_devices[i].name));
        storage_devices[i].type = STORAGE_HDD;
        storage_devices[i].capacity_mb = 0;
        storage_devices[i].online = 0;
    }
    storage_device_count = 0;

    g_fs_ata_enabled = (uint8_t)ata_pio_init();
    memset(disk_storage, 0, sizeof(disk_storage));
    if (fs_load_metadata()) {
        // Keep existing persisted filesystem image.
    } else {
        next_disk_sector = FS_DATA_START_SECTOR;

        fs_create_directory("C:/");
        fs_create_directory("C:/GamerOS");
        fs_create_directory("C:/GamerOS/System32");
        fs_create_directory("C:/Users");
        fs_create_directory("C:/Program Files");
        fs_create_directory("C:/Apps");
    }

    // Keep the shell on a single stable logical volume while storage init and
    // app launching are still being hardened for VMware.
    strncpy(storage_devices[0].name, g_fs_ata_enabled ? "System Disk" : "RAM System Disk",
            sizeof(storage_devices[0].name) - 1);
    storage_devices[0].name[sizeof(storage_devices[0].name) - 1] = 0;
    storage_devices[0].type = g_fs_ata_enabled ? STORAGE_HDD : STORAGE_RAMDISK;
    storage_devices[0].capacity_mb = g_fs_ata_enabled ? 102400 : 256;
    storage_devices[0].online = 1;

    storage_device_count = 1;
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
            if (next_disk_sector + FS_FILE_RESERVED_SECTORS >= MAX_DISK_SECTORS) {
                files[i].in_use = 0;
                return 0;
            }
            files[i].disk_sector = next_disk_sector;
            next_disk_sector += FS_FILE_RESERVED_SECTORS;
            fs_save_metadata();

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
    if (!file || !file->in_use || !data || size > MAX_FILE_SIZE) return;

    // Keep memory cache for fast reads and UI code.
    memcpy(file->data, data, size);
    file->size = size;

    // Real persistent write path (ATA PIO if available).
    uint8_t sector[DISK_SECTOR_SIZE];
    uint32_t sector_count = (size + DISK_SECTOR_SIZE - 1) / DISK_SECTOR_SIZE;
    if (sector_count > FS_FILE_RESERVED_SECTORS) {
        sector_count = FS_FILE_RESERVED_SECTORS;
        file->size = FS_FILE_RESERVED_SECTORS * DISK_SECTOR_SIZE;
    }

    for (uint32_t si = 0; si < sector_count; si++) {
        uint32_t copy_off = si * DISK_SECTOR_SIZE;
        uint32_t left = file->size - copy_off;
        uint32_t copy_size = left < DISK_SECTOR_SIZE ? left : DISK_SECTOR_SIZE;
        memset(sector, 0, sizeof(sector));
        memcpy(sector, data + copy_off, copy_size);
        (void)fs_disk_write_sector(file->disk_sector + si, sector);
    }
    for (uint32_t si = sector_count; si < FS_FILE_RESERVED_SECTORS; si++) {
        memset(sector, 0, sizeof(sector));
        (void)fs_disk_write_sector(file->disk_sector + si, sector);
    }
    fs_save_metadata();
}

void fs_read_file(file_t* file, uint8_t* buffer, uint32_t size) {
    if (!file || !file->in_use || !buffer || size == 0) return;

    uint32_t bytes_to_read = (size < file->size) ? size : file->size;
    uint32_t copied = 0;
    uint8_t sector[DISK_SECTOR_SIZE];
    while (copied < bytes_to_read) {
        uint32_t si = copied / DISK_SECTOR_SIZE;
        uint32_t in_sector = copied % DISK_SECTOR_SIZE;
        if (si >= FS_FILE_RESERVED_SECTORS) break;
        if (!fs_disk_read_sector(file->disk_sector + si, sector)) break;
        uint32_t chunk = DISK_SECTOR_SIZE - in_sector;
        if (chunk > (bytes_to_read - copied)) chunk = bytes_to_read - copied;
        memcpy(buffer + copied, sector + in_sector, chunk);
        copied += chunk;
    }

    if (copied < bytes_to_read) {
        // Fallback if disk read failed mid-way.
        memcpy(buffer + copied, file->data + copied, bytes_to_read - copied);
    }

    if (size > bytes_to_read) {
        memset(buffer + bytes_to_read, 0, size - bytes_to_read);
    }

    if (bytes_to_read > 0 && bytes_to_read <= MAX_FILE_SIZE) {
        // Refresh cache from persisted data.
        memcpy(file->data, buffer, bytes_to_read);
        if (file->size > bytes_to_read) {
            memset(file->data + bytes_to_read, 0, file->size - bytes_to_read);
        }
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
            fs_save_metadata();
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
    fs_save_metadata();
}

// TODO: Implement directory creation, removal, and listing
// TODO: Add file permissions and access control
// TODO: Implement file system journaling for crash recovery
// TODO: Add support for file attributes (hidden, system, read-only)
