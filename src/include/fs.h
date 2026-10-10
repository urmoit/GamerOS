#ifndef FS_H
#define FS_H

#include "stdint.h"

#define MAX_FILES 256
// Was 40. Seeding writes paths such as
// "C:/GamerOS/Apps/Manifests/Notepad.gosapp", which is exactly 40 characters.
// At the old limit strncpy truncated the stored name, so the next boot no longer
// matched it and fs_create_file appended a duplicate entry on every boot.
#define MAX_FILENAME_LEN 64
// 24KB per file. The changelog viewer ships a full release changelog in one
// file, which overflowed the old 12KB cap.
#define MAX_FILE_SIZE 24576
#define MAX_DIRECTORIES 128
#define MAX_STORAGE_DEVICES 8

typedef struct {
    char path[MAX_FILENAME_LEN];
    uint8_t in_use;
} directory_t;

// File contents live on disk only. The previous layout embedded a full
// MAX_FILE_SIZE buffer per slot, which cost 32 * 24KB = 768KB of BSS to hold
// data that belongs on the medium.
typedef struct {
    char name[MAX_FILENAME_LEN];
    uint32_t size;
    uint8_t in_use;
    uint32_t disk_sector;      // kept for compatibility with older callers
    uint64_t lba;              // absolute start LBA on the medium
    uint32_t sector_count;     // sectors currently allocated to this file
    uint32_t mtime;            // Unix seconds; drives the "Date modified" column
} file_t;

typedef enum {
    STORAGE_HDD = 0,
    STORAGE_SSD = 1,
    STORAGE_NVME = 2,
    STORAGE_USB = 3,
    STORAGE_CDROM = 4,
    STORAGE_RAMDISK = 5
} storage_device_type_t;

typedef struct {
    char name[20];
    char label[40];
    storage_device_type_t type;
    uint32_t capacity_mb;
    uint64_t total_bytes;
    uint64_t free_bytes;
    uint64_t used_bytes;
    int persistent;   // 1 = backed by a real medium, 0 = RAM only
    uint8_t online;
} storage_device_info_t;

// Capacity accounting for the whole volume, used by Explorer to draw the
// drive usage bar.
typedef struct {
    uint64_t total_sectors;
    uint64_t used_sectors;
    uint64_t free_sectors;
    uint64_t total_bytes;
    uint64_t used_bytes;
    uint64_t free_bytes;
    uint32_t file_count;
    uint32_t dir_count;
    int persistent;
} fs_space_info_t;

void fs_init();
file_t* fs_create_file(const char* name);
file_t* fs_open_file(const char* name);
void fs_write_file(file_t* file, const uint8_t* data, uint32_t size);
void fs_read_file(file_t* file, uint8_t* buffer, uint32_t size);
int fs_create_directory(const char* path);
int fs_delete_file(const char* path);
int fs_delete_directory(const char* path);
int fs_directory_exists(const char* path);
int fs_file_exists(const char* path);
uint32_t fs_get_file_size(const char* path);
uint32_t fs_get_file_mtime(const char* path);
// Metadata for one entry, so the details view can render a whole row without
// reopening the file. is_dir selects the folder interpretation when the name is
// not a known file.
int fs_get_entry_info(const char* path, int is_dir, uint32_t* out_size, uint32_t* out_mtime);
int fs_list_entries(const char* directory, char out_names[][MAX_FILENAME_LEN], uint8_t out_is_dir[], int max_entries);
int fs_storage_get_device_count(void);
int fs_storage_get_device(int index, storage_device_info_t* out);
int fs_get_space_info(fs_space_info_t* out);
void fs_sync(void);

#endif
