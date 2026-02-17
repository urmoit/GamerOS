#ifndef APPS_H
#define APPS_H

#include "stdint.h"

#define APP_WINDOW_NONE 0
#define APP_WINDOW_NOTEPAD 1
#define APP_WINDOW_MYCOMP 2
#define APP_WINDOW_ABOUT 3
#define APP_WINDOW_SETTINGS 4
#define APP_WINDOW_EXPLORER 5

typedef struct {
    const char* exe_name;
    const char* system_path;
    const char* display_name;
    int window_type;
    int launch_x;
    int launch_y;
} app_descriptor_t;

int apps_get_count(void);
const app_descriptor_t* apps_get_by_index(int idx);
const app_descriptor_t* apps_find_by_exe(const char* exe_name);
int apps_resolve_launch(const char* exe_name, int* out_window_type, int* out_x, int* out_y);

#endif
