#include "../intf/apps.h"
#include "../intf/string.h"

static const app_descriptor_t g_apps[] = {
    {
        "NOTEPAD.EXE",
        "C:/GamerOS/System32/NOTEPAD.EXE",
        "Notepad",
        APP_WINDOW_NOTEPAD,
        42,
        22
    },
    {
        "SETTINGS.EXE",
        "C:/GamerOS/System32/SETTINGS.EXE",
        "Settings",
        APP_WINDOW_SETTINGS,
        24,
        14
    },
    {
        "EXPLORER.EXE",
        "C:/GamerOS/System32/EXPLORER.EXE",
        "File Explorer",
        APP_WINDOW_EXPLORER,
        28,
        18
    }
};

static int path_matches_exe(const char* path, const char* exe_name) {
    if (!path || !exe_name) return 0;
    if (strcmp(path, exe_name) == 0) return 1;

    size_t path_len = strlen(path);
    size_t exe_len = strlen(exe_name);
    if (path_len < exe_len) return 0;
    return strcmp(path + (path_len - exe_len), exe_name) == 0;
}

int apps_get_count(void) {
    return (int)(sizeof(g_apps) / sizeof(g_apps[0]));
}

const app_descriptor_t* apps_get_by_index(int idx) {
    if (idx < 0 || idx >= apps_get_count()) return 0;
    return &g_apps[idx];
}

const app_descriptor_t* apps_find_by_exe(const char* exe_name) {
    if (!exe_name || !exe_name[0]) return 0;
    int count = apps_get_count();
    for (int i = 0; i < count; i++) {
        if (path_matches_exe(exe_name, g_apps[i].exe_name) ||
            path_matches_exe(exe_name, g_apps[i].system_path)) {
            return &g_apps[i];
        }
    }
    return 0;
}

int apps_resolve_launch(const char* exe_name, int* out_window_type, int* out_x, int* out_y) {
    const app_descriptor_t* app = apps_find_by_exe(exe_name);
    if (!app) return 0;
    if (out_window_type) *out_window_type = app->window_type;
    if (out_x) *out_x = app->launch_x;
    if (out_y) *out_y = app->launch_y;
    return 1;
}
