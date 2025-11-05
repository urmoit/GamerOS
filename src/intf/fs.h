#ifndef FS_H
#define FS_H

#include "stdint.h"

#define MAX_FILES 16
#define MAX_FILENAME_LEN 16
#define MAX_FILE_SIZE 256

typedef struct {
    char name[MAX_FILENAME_LEN];
    uint8_t  data[MAX_FILE_SIZE];
    uint32_t size;
    uint8_t  in_use;
} file_t;

void fs_init();
file_t* fs_create_file(const char* name);
file_t* fs_open_file(const char* name);
void fs_write_file(file_t* file, const uint8_t* data, uint32_t size);
void fs_read_file(file_t* file, uint8_t* buffer, uint32_t size);

#endif