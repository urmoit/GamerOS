#ifndef FS_H
#define FS_H

#include "stdint.h"

#define MAX_FILES 32
#define MAX_FILENAME_LEN 40
#define MAX_FILE_SIZE 256
#define MAX_DIRECTORIES 32
#define MAX_STORAGE_DEVICES 8

typedef struct {
    char path[MAX_FILENAME_LEN];
    uint8_t in_use;
} directory_t;

typedef struct {
    char name[MAX_FILENAME_LEN];
    uint8_t  data[MAX_FILE_SIZE];
    uint32_t size;
    uint8_t  in_use;
    uint32_t disk_sector;
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
    storage_device_type_t type;
    uint32_t capacity_mb;
    uint8_t online;
} storage_device_info_t;

void fs_init();
file_t* fs_create_file(const char* name);
file_t* fs_open_file(const char* name);
void fs_write_file(file_t* file, const uint8_t* data, uint32_t size);
void fs_read_file(file_t* file, uint8_t* buffer, uint32_t size);
int fs_create_directory(const char* path);
int fs_directory_exists(const char* path);
int fs_file_exists(const char* path);
uint32_t fs_get_file_size(const char* path);
int fs_list_entries(const char* directory, char out_names[][MAX_FILENAME_LEN], uint8_t out_is_dir[], int max_entries);
int fs_storage_get_device_count(void);
int fs_storage_get_device(int index, storage_device_info_t* out);

#endif
