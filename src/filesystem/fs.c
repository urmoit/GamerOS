#include <fs.h>
#include <ata_pio.h>
#include <stdint.h>
#include <rtc.h>
#include <string.h>

#define DISK_SECTOR_SIZE ATA_SECTOR_SIZE

// First 64 sectors (one legacy track) stay untouched so any MBR/GPT survives.
#define FS_RESERVED_SECTORS 64

#define FS_FILE_RESERVED_SECTORS ((MAX_FILE_SIZE + DISK_SECTOR_SIZE - 1) / DISK_SECTOR_SIZE)

// RAM fallback when no ATA device answers: 2 MiB of BSS.
#define FS_RAM_TOTAL_SECTORS 4096

#define FS_SUPERBLOCK_SECTOR FS_RESERVED_SECTORS
#define FS_FILE_TABLE_START (FS_SUPERBLOCK_SECTOR + 1)
#define FS_FILE_TABLE_SECTORS 64
#define FS_DIR_TABLE_START (FS_FILE_TABLE_START + FS_FILE_TABLE_SECTORS)
#define FS_DIR_TABLE_SECTORS 24
#define FS_DATA_START_SECTOR (FS_DIR_TABLE_START + FS_DIR_TABLE_SECTORS)

// File record is 88 bytes (5 per sector), dir record 72 bytes (7 per sector).
#define FS_MAGIC "GFS3PIO"
#define FS_VERSION 3

static uint8_t disk_storage[FS_RAM_TOTAL_SECTORS * DISK_SECTOR_SIZE];
static uint8_t g_fs_ata_enabled = 0;
static uint64_t g_fs_total_sectors = FS_RAM_TOTAL_SECTORS;

typedef struct {
    char magic[8];
    uint32_t version;
    uint64_t next_disk_sector;
    uint64_t total_sectors;
    uint32_t reserved0;
    uint32_t reserved1;
    uint8_t padding[DISK_SECTOR_SIZE - 32];
} fs_superblock_t;

typedef struct {
    char name[MAX_FILENAME_LEN];
    uint32_t size;
    uint32_t sector_count;
    uint64_t lba;
    uint8_t in_use;
    // mtime reuses former padding, so the record stays 88 bytes (old images read 0).
    uint8_t reserved[3];
    uint32_t mtime;
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
static uint64_t next_disk_sector = FS_DATA_START_SECTOR;

static int fs_disk_read_sector(uint64_t lba, uint8_t* out) {
    if (!out) return 0;
    if (lba >= g_fs_total_sectors) return 0;
    if (g_fs_ata_enabled) return ata_pio_read_sector(lba, out);
    memcpy(out, &disk_storage[lba * DISK_SECTOR_SIZE], DISK_SECTOR_SIZE);
    return 1;
}

static int fs_disk_write_sector(uint64_t lba, const uint8_t* in) {
    if (!in) return 0;
    if (lba >= g_fs_total_sectors) return 0;
    if (g_fs_ata_enabled) return ata_pio_write_sector(lba, in);
    memcpy(&disk_storage[lba * DISK_SECTOR_SIZE], in, DISK_SECTOR_SIZE);
    return 1;
}

static uint64_t fs_used_sectors(void) {
    if (next_disk_sector <= FS_DATA_START_SECTOR) return 0;
    return next_disk_sector - FS_DATA_START_SECTOR;
}

static void fs_reset_tables(void) {
    for (size_t i = 0; i < MAX_FILES; i++) {
        files[i].in_use = 0;
        files[i].size = 0;
        files[i].lba = 0;
        files[i].sector_count = 0;
        files[i].disk_sector = 0;
        memset(files[i].name, 0, MAX_FILENAME_LEN);
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
    memcpy(sb.magic, FS_MAGIC, 8);
    sb.version = FS_VERSION;
    sb.next_disk_sector = next_disk_sector;
    sb.total_sectors = g_fs_total_sectors;

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
            rec.sector_count = files[idx].sector_count;
            rec.lba = files[idx].lba;
            rec.in_use = files[idx].in_use;
            rec.mtime = files[idx].mtime;
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
    if (next_disk_sector > g_fs_total_sectors) next_disk_sector = FS_DATA_START_SECTOR;

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
            files[idx].sector_count = rec.sector_count;
            files[idx].lba = rec.lba;
            files[idx].disk_sector = (uint32_t)rec.lba;
            files[idx].in_use = rec.in_use;
            files[idx].mtime = rec.mtime;
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
        memset(storage_devices[i].label, 0, sizeof(storage_devices[i].label));
        storage_devices[i].type = STORAGE_HDD;
        storage_devices[i].capacity_mb = 0;
        storage_devices[i].online = 0;
    }
    storage_device_count = 0;

    g_fs_ata_enabled = (uint8_t)ata_pio_init();
    if (g_fs_ata_enabled) {
        g_fs_total_sectors = ata_pio_get_capacity_sectors();
        if (g_fs_total_sectors <= FS_DATA_START_SECTOR) g_fs_total_sectors = FS_DATA_START_SECTOR + FS_RAM_TOTAL_SECTORS;
    } else {
        g_fs_total_sectors = FS_RAM_TOTAL_SECTORS;
    }

    memset(disk_storage, 0, sizeof(disk_storage));

    if (!fs_load_metadata()) {
        next_disk_sector = FS_DATA_START_SECTOR;

        fs_create_directory("C:/");
        fs_create_directory("C:/GamerOS");
        fs_create_directory("C:/GamerOS/System32");
        fs_create_directory("C:/Users");
        fs_create_directory("C:/Users/Admin");
        fs_create_directory("C:/Users/Admin/Documents");
        fs_create_directory("C:/Users/Admin/Downloads");
        fs_create_directory("C:/Program Files");
        fs_create_directory("C:/Apps");
    }

    // One logical volume, labelled from whatever the medium reports.
    storage_device_info_t* dev = &storage_devices[0];
    memset(dev, 0, sizeof(*dev));
    if (g_fs_ata_enabled) {
        const char* model = ata_pio_get_model();
        strncpy(dev->label, (model && model[0]) ? model : "GamerOS Volume",
                sizeof(dev->label) - 1);
    } else {
        strncpy(dev->label, "GamerOS Volume (RAM)", sizeof(dev->label) - 1);
    }
    strncpy(dev->name, "System Disk", sizeof(dev->name) - 1);
    dev->type = g_fs_ata_enabled ? STORAGE_HDD : STORAGE_RAMDISK;
    dev->persistent = g_fs_ata_enabled;
    dev->total_bytes = g_fs_total_sectors * DISK_SECTOR_SIZE;
    dev->used_bytes = fs_used_sectors() * DISK_SECTOR_SIZE;
    dev->free_bytes = dev->total_bytes - dev->used_bytes;
    dev->capacity_mb = (uint32_t)(dev->total_bytes >> 20);
    dev->online = 1;

    storage_device_count = 1;
}

file_t* fs_create_file(const char* name) {
    if (!name || !name[0]) return 0;
    if (strlen(name) >= MAX_FILENAME_LEN) return 0;
    if (fs_open_file(name)) return 0;

    for (size_t i = 0; i < MAX_FILES; i++) {
        if (files[i].in_use) continue;
        strncpy(files[i].name, name, MAX_FILENAME_LEN - 1);
        files[i].name[MAX_FILENAME_LEN - 1] = '\0';
        files[i].in_use = 1;
        files[i].size = 0;
        files[i].lba = 0;
        files[i].sector_count = 0;
        files[i].disk_sector = 0;
        fs_save_metadata();
        return &files[i];
    }
    return 0;
}

file_t* fs_open_file(const char* name) {
    if (!name) return 0;
    for (size_t i = 0; i < MAX_FILES; i++) {
        if (files[i].in_use && strcmp(files[i].name, name) == 0) {
            return &files[i];
        }
    }
    return 0;
}

// Append-only allocator: freed space is not reused until the volume is reformatted.
static int fs_alloc_run(uint32_t needed, uint64_t* out_lba) {
    if (needed == 0) { *out_lba = 0; return 1; }
    if (needed > FS_FILE_RESERVED_SECTORS) return 0;
    if (next_disk_sector + needed > g_fs_total_sectors) return 0;
    *out_lba = next_disk_sector;
    next_disk_sector += needed;
    return 1;
}

void fs_write_file(file_t* file, const uint8_t* data, uint32_t size) {
    if (!file || !file->in_use || (!data && size > 0)) return;
    if (size > MAX_FILE_SIZE) size = MAX_FILE_SIZE;

    uint32_t needed = (size + DISK_SECTOR_SIZE - 1) / DISK_SECTOR_SIZE;

    if (needed > file->sector_count) {
        uint64_t lba = 0;
        if (!fs_alloc_run(needed, &lba)) return;
        file->lba = lba;
        file->disk_sector = (uint32_t)lba;
        file->sector_count = needed;
    }

    uint8_t sector[DISK_SECTOR_SIZE];
    for (uint32_t si = 0; si < needed; si++) {
        uint32_t off = si * DISK_SECTOR_SIZE;
        uint32_t left = size - off;
        uint32_t chunk = left < DISK_SECTOR_SIZE ? left : DISK_SECTOR_SIZE;
        memset(sector, 0, sizeof(sector));
        if (chunk) memcpy(sector, data + off, chunk);
        (void)fs_disk_write_sector(file->lba + si, sector);
    }

    for (uint32_t si = needed; si < file->sector_count; si++) {
        memset(sector, 0, sizeof(sector));
        (void)fs_disk_write_sector(file->lba + si, sector);
    }
    if (needed < file->sector_count) file->sector_count = needed;

    file->size = size;
    file->mtime = get_unix_time();
    fs_save_metadata();
}

void fs_read_file(file_t* file, uint8_t* buffer, uint32_t size) {
    if (!file || !file->in_use || !buffer || size == 0) return;

    uint32_t bytes_to_read = (size < file->size) ? size : file->size;
    uint32_t copied = 0;
    uint8_t sector[DISK_SECTOR_SIZE];

    while (copied < bytes_to_read && copied / DISK_SECTOR_SIZE < file->sector_count) {
        uint32_t si = copied / DISK_SECTOR_SIZE;
        uint32_t in_sector = copied % DISK_SECTOR_SIZE;
        if (!fs_disk_read_sector(file->lba + si, sector)) break;
        uint32_t chunk = DISK_SECTOR_SIZE - in_sector;
        if (chunk > (bytes_to_read - copied)) chunk = bytes_to_read - copied;
        memcpy(buffer + copied, sector + in_sector, chunk);
        copied += chunk;
    }

    if (size > bytes_to_read) {
        memset(buffer + bytes_to_read, 0, size - bytes_to_read);
    }
}

int fs_create_directory(const char* path) {
    if (!path || !path[0]) return 0;
    if (strlen(path) >= MAX_FILENAME_LEN) return 0;
    if (fs_directory_exists(path)) return 1;
    for (size_t i = 0; i < MAX_DIRECTORIES; i++) {
        if (directories[i].in_use) continue;
        directories[i].in_use = 1;
        strncpy(directories[i].path, path, MAX_FILENAME_LEN - 1);
        directories[i].path[MAX_FILENAME_LEN - 1] = 0;
        fs_save_metadata();
        return 1;
    }
    return 0;
}

int fs_delete_file(const char* path) {
    if (!path) return 0;
    for (size_t i = 0; i < MAX_FILES; i++) {
        if (!files[i].in_use) continue;
        if (!paths_equal(files[i].name, path)) continue;

        uint8_t sector[DISK_SECTOR_SIZE];
        for (uint32_t si = 0; si < files[i].sector_count; si++) {
            memset(sector, 0, sizeof(sector));
            (void)fs_disk_write_sector(files[i].lba + si, sector);
        }

        files[i].in_use = 0;
        files[i].size = 0;
        files[i].lba = 0;
        files[i].sector_count = 0;
        files[i].disk_sector = 0;
        memset(files[i].name, 0, MAX_FILENAME_LEN);
        fs_save_metadata();
        return 1;
    }
    return 0;
}

int fs_delete_directory(const char* path) {
    if (!path) return 0;
    for (size_t i = 0; i < MAX_DIRECTORIES; i++) {
        if (!directories[i].in_use) continue;
        if (!paths_equal(directories[i].path, path)) continue;
        directories[i].in_use = 0;
        memset(directories[i].path, 0, MAX_FILENAME_LEN);
        fs_save_metadata();
        return 1;
    }
    return 0;
}

int fs_directory_exists(const char* path) {
    if (!path) return 0;
    for (size_t i = 0; i < MAX_DIRECTORIES; i++) {
        if (directories[i].in_use && paths_equal(directories[i].path, path)) return 1;
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

uint32_t fs_get_file_mtime(const char* path) {
    file_t* file = fs_open_file(path);
    if (!file) return 0;
    return file->mtime;
}

int fs_get_entry_info(const char* path, int is_dir, uint32_t* out_size, uint32_t* out_mtime) {
    if (!path) return 0;
    if (is_dir) {
        // Folders have no payload; stamp mtime with the current clock.
        if (out_size) *out_size = 0;
        if (out_mtime) *out_mtime = get_unix_time();
        return 1;
    }
    file_t* file = fs_open_file(path);
    if (!file) {
        if (out_size) *out_size = 0;
        if (out_mtime) *out_mtime = 0;
        return 0;
    }
    if (out_size) *out_size = file->size;
    if (out_mtime) *out_mtime = file->mtime;
    return 1;
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

int fs_get_space_info(fs_space_info_t* out) {
    if (!out) return 0;
    memset(out, 0, sizeof(*out));

    out->total_sectors = g_fs_total_sectors;
    out->used_sectors = fs_used_sectors();
    out->free_sectors = g_fs_total_sectors - FS_DATA_START_SECTOR - out->used_sectors;
    out->total_bytes = g_fs_total_sectors * DISK_SECTOR_SIZE;
    out->used_bytes = out->used_sectors * DISK_SECTOR_SIZE;
    out->free_bytes = out->free_sectors * DISK_SECTOR_SIZE;
    out->persistent = g_fs_ata_enabled;

    for (size_t i = 0; i < MAX_FILES; i++) if (files[i].in_use) out->file_count++;
    for (size_t i = 0; i < MAX_DIRECTORIES; i++) if (directories[i].in_use) out->dir_count++;

    if (storage_device_count > 0) {
        storage_devices[0].used_bytes = out->used_bytes;
        storage_devices[0].free_bytes = out->free_bytes;
    }
    return 1;
}

void fs_sync(void) {
    fs_save_metadata();
}
