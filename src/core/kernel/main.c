#include <graphics.h>
#include <mouse.h>
#include <keyboard.h>
#include <serial.h>
#include <io.h>
#include <ports.h>
#include <rtc.h>
#include <font.h>
#include <fs.h>
#include <apps.h>
#include <process_model.h>
#include <multiboot.h>
#include <notepad/notepad_ui.h>
#include <settings/settings_ui.h>
#include <explorer/explorer_ui.h>
#include "../../resources/wallpapers/background_wallpaper.h"
#include "stdint.h"
#include "string.h"

// Window states
#define MAX_WINDOWS 6
#define WIN_NONE    0
#define WIN_NOTEPAD APP_WINDOW_NOTEPAD
#define WIN_MYCOMP  APP_WINDOW_MYCOMP
#define WIN_ABOUT   APP_WINDOW_ABOUT
#define WIN_SETTINGS APP_WINDOW_SETTINGS
#define WIN_EXPLORER APP_WINDOW_EXPLORER

// Desktop shell sizing (compact XP-like layout for 320x200)
#define TASKBAR_HEIGHT         20
#define START_BTN_X            4
#define START_BTN_W            52
#define START_BTN_H            16
#define DESKTOP_ICON_W         20
#define DESKTOP_ICON_H         20
#define DESKTOP_ICON_HIT_W     28
#define DESKTOP_ICON_HIT_H     32
#define TASKBTN_W              70
#define TASKBTN_H              16
#define START_MENU_X           0
#define START_MENU_W           220
#define START_MENU_H           164
#define START_MENU_ITEM_H      20
#define WINDOW_RESIZE_GRIP     12
#define WINDOW_MIN_W_DEFAULT   140
#define WINDOW_MIN_H_DEFAULT   90

typedef struct {
    int active;
    int type;
    int x, y;
    int w, h;
    int dragging;
    int resizing;
    int drag_x, drag_y;
    int resize_start_w, resize_start_h;
    int resize_start_mx, resize_start_my;
    char title[32];
} window_t;

static window_t windows[MAX_WINDOWS];
static int active_window = -1;
static int window_count = 0;

// Notepad content
#define NOTEPAD_MAX_LINES 160
#define NOTEPAD_MAX_COLS  96
static char notepad_lines[NOTEPAD_MAX_LINES][NOTEPAD_MAX_COLS];
static int notepad_cursor_x = 0;
static int notepad_cursor_y = 0;
static int notepad_view_top = 0;
static uint8_t notepad_dirty = 0;
// Use a virtual GamerOS-style path for Notepad data; this is resolved
// into a concrete filesystem path by the GOS path helpers below.
#define NOTEPAD_FILE_PATH "GOS:/User/Notepad/NOTEPAD.TXT"

// Input state
static uint8_t last_buttons = 0;
static uint8_t start_menu_open = 0;
static uint8_t desktop_menu_open = 0;
static int desktop_menu_x = 0;
static int desktop_menu_y = 0;
static uint8_t shutdown_requested = 0;
static uint8_t error_popup_open = 0;
static char error_popup_title[40];
static char error_popup_message[160];
static uint8_t warned_mouse_packet = 0;
static uint8_t warned_mouse_bounds = 0;
static uint8_t setting_show_seconds = 0;
static uint8_t setting_desktop_glow = 0;
static uint8_t setting_compact_mode = 1;
static int setting_display_profile = 0;
static int settings_tab = 0;

#define SETTINGS_TAB_SYSTEM 0
#define SETTINGS_TAB_BLUETOOTH_DEVICES 1
#define SETTINGS_TAB_NETWORK_INTERNET 2
#define SETTINGS_TAB_PERSONALIZATION 3
#define SETTINGS_TAB_APPS 4
#define SETTINGS_TAB_ACCOUNTS 5
#define SETTINGS_TAB_TIME_LANGUAGE 6
#define SETTINGS_TAB_GAMING 7
#define SETTINGS_TAB_ACCESSIBILITY 8
#define SETTINGS_TAB_PRIVACY_SECURITY 9
#define SETTINGS_TAB_GAMEROS_UPDATE 10
#define SETTINGS_TAB_COUNT 11
static int settings_scroll_top[SETTINGS_TAB_COUNT] = {0};
typedef struct {
    char text[120];
    uint8_t color;
    uint8_t indent;
} markdown_line_t;
#define SETTINGS_MD_MAX_LINES 256
static markdown_line_t settings_md_lines[SETTINGS_MD_MAX_LINES];
static int settings_md_line_count = 0;
static uint8_t settings_md_loaded = 0;
static int settings_md_plan_start = -1;

typedef enum {
    SETTINGS_UPDATE_VIEW_CHANGELOG = 0,
    SETTINGS_UPDATE_VIEW_ROADMAP = 1
} settings_update_view_t;

static int settings_update_view = SETTINGS_UPDATE_VIEW_CHANGELOG;

#define EXPLORER_MAX_ENTRIES 14
static char explorer_path[MAX_FILENAME_LEN] = "C:/";
static char explorer_entries[EXPLORER_MAX_ENTRIES][MAX_FILENAME_LEN];
static uint8_t explorer_is_dir[EXPLORER_MAX_ENTRIES];
static int explorer_entry_count = 0;
static int explorer_selected = -1;
static uint8_t explorer_this_pc_view = 1;
static char explorer_drive_paths[EXPLORER_MAX_ENTRIES][4];
static char explorer_drive_labels[EXPLORER_MAX_ENTRIES][32];
static int explorer_drive_count = 0;
static uint8_t storage_initialized = 0;
static uint32_t cursor_under_rgb[CURSOR_SIZE * CURSOR_SIZE];
static uint8_t notepad_io_buffer[MAX_FILE_SIZE + 1];
static int32_t cursor_drawn_x = 0;
static int32_t cursor_drawn_y = 0;
static uint8_t cursor_drawn_valid = 0;

#define DEBUG_LOG_LINES 128
#define DEBUG_LOG_CHARS 160
static char debug_log_lines[DEBUG_LOG_LINES][DEBUG_LOG_CHARS];
static int debug_log_count = 0;
static uint8_t debug_overlay_enabled = 0;
static int debug_overlay_x = 4;
static int debug_overlay_y = 4;
static int debug_overlay_w = 8 + (DEBUG_LOG_CHARS * 6);
static int debug_overlay_h = 112;
static uint8_t debug_overlay_dragging = 0;
static uint8_t debug_overlay_resizing = 0;
static int debug_overlay_drag_x = 0;
static int debug_overlay_drag_y = 0;
static int debug_overlay_resize_start_w = 0;
static int debug_overlay_resize_start_h = 0;
static int debug_overlay_resize_start_mx = 0;
static int debug_overlay_resize_start_my = 0;

// Simple app lifecycle tracking so the shell has a notion of which
// first-class apps are running or stopped.
typedef enum {
    APP_STATE_STOPPED = 0,
    APP_STATE_RUNNING = 1,
    APP_STATE_SUSPENDED = 2
} app_state_t;

typedef struct {
    const app_descriptor_t* descriptor;
    app_state_t state;
} app_runtime_entry_t;

#define MAX_APP_RUNTIME_SLOTS 8
static app_runtime_entry_t g_app_runtime[MAX_APP_RUNTIME_SLOTS];
static int g_app_runtime_count = 0;

static app_runtime_entry_t* app_runtime_find(const app_descriptor_t* descriptor) {
    if (!descriptor) return 0;
    for (int i = 0; i < g_app_runtime_count; i++) {
        if (g_app_runtime[i].descriptor == descriptor) {
            return &g_app_runtime[i];
        }
    }
    return 0;
}

static app_runtime_entry_t* app_runtime_get_or_create(const app_descriptor_t* descriptor) {
    if (!descriptor) return 0;
    app_runtime_entry_t* entry = app_runtime_find(descriptor);
    if (entry) return entry;
    if (g_app_runtime_count >= MAX_APP_RUNTIME_SLOTS) {
        return 0;
    }
    entry = &g_app_runtime[g_app_runtime_count++];
    entry->descriptor = descriptor;
    entry->state = APP_STATE_STOPPED;
    return entry;
}

static void app_lifecycle_mark_launched(const app_descriptor_t* descriptor) {
    app_runtime_entry_t* entry = app_runtime_get_or_create(descriptor);
    if (!entry) return;
    entry->state = APP_STATE_RUNNING;
}

static void app_lifecycle_mark_closed_by_type(int win_type) {
    if (win_type == WIN_NONE) return;

    // Find the app descriptor that owns this window type.
    const app_descriptor_t* descriptor = 0;
    int app_count = apps_get_count();
    for (int i = 0; i < app_count; i++) {
        const app_descriptor_t* candidate = apps_get_by_index(i);
        if (candidate && candidate->window_type == win_type) {
            descriptor = candidate;
            break;
        }
    }
    if (!descriptor) return;

    app_runtime_entry_t* entry = app_runtime_find(descriptor);
    if (!entry) return;
    entry->state = APP_STATE_STOPPED;
}

// High-level GamerOS virtual path helper for app and user storage.
// Maps prefixes like:
//   GOS:/User/...   -> C:/Users/Admin/...
//   GOS:/System/... -> C:/GamerOS/System32/...
//   GOS:/Apps/...   -> C:/GamerOS/Apps/...
static int gos_path_resolve(const char* gos_path, char* out_fs_path, size_t out_cap) {
    if (!gos_path || !out_fs_path || out_cap == 0) return 0;

    // If this is already a concrete path, just copy it through.
    if (strncmp(gos_path, "GOS:/", 5) != 0) {
        strncpy(out_fs_path, gos_path, out_cap - 1);
        out_fs_path[out_cap - 1] = 0;
        return 1;
    }

    const char* rest = gos_path + 5;
    const char* base_fs = 0;

    if (strncmp(rest, "User/", 5) == 0) {
        base_fs = "C:/Users/Admin/";
        rest += 5;
    } else if (strncmp(rest, "System/", 7) == 0) {
        base_fs = "C:/GamerOS/System32/";
        rest += 7;
    } else if (strncmp(rest, "Apps/", 5) == 0) {
        base_fs = "C:/GamerOS/Apps/";
        rest += 5;
    } else {
        // Fallback to the root of C: if the sub-prefix is unknown.
        base_fs = "C:/";
    }

    size_t base_len = strlen(base_fs);
    size_t rest_len = strlen(rest);
    if (base_len + rest_len + 1 > out_cap) {
        if (out_cap <= base_len + 1) {
            // Not enough room even for the base path and terminator.
            return 0;
        }
        rest_len = out_cap - base_len - 1;
    }

    memcpy(out_fs_path, base_fs, base_len);
    memcpy(out_fs_path + base_len, rest, rest_len);
    out_fs_path[base_len + rest_len] = 0;
    return 1;
}

static file_t* gos_fs_open_file(const char* gos_path) {
    char fs_path[MAX_FILENAME_LEN];
    if (!gos_path_resolve(gos_path, fs_path, sizeof(fs_path))) {
        return 0;
    }
    return fs_open_file(fs_path);
}

static file_t* gos_fs_create_file(const char* gos_path) {
    char fs_path[MAX_FILENAME_LEN];
    if (!gos_path_resolve(gos_path, fs_path, sizeof(fs_path))) {
        return 0;
    }
    return fs_create_file(fs_path);
}

static int taskbar_clock_width(void) {
    int time_chars = setting_show_seconds ? 8 : 5; // HH:MM:SS or HH:MM
    int date_chars = 10; // DD/MM/YYYY
    return ((time_chars + 1 + date_chars) * 6) + 8;
}

typedef struct {
    int x;
    int y;
    int w;
    int h;
    int item_h;
} start_menu_metrics_t;

typedef struct {
    int x;
    int y;
    int w;
    int h;
    int item_h;
} desktop_menu_metrics_t;

static void append_string(char* dest, size_t dest_cap, const char* src);
static void initialize_storage_layout(void);
static file_t* fs_open_or_create_file(const char* path);
static void write_text_file(const char* path, const char* txt);
static void apply_display_profile(void);
static void debug_log_message(const char* message);
static void draw_debug_overlay(void);
static void debug_present_boot_status(void);
static void get_debug_overlay_rect(int* out_x, int* out_y, int* out_w, int* out_h);
static void get_window_min_size(const window_t* win, int* out_w, int* out_h);
static void draw_desktop_watermark(void);
static void draw_desktop_wallpaper(int desktop_h);
static int get_ui_scale(void);
static void get_start_menu_metrics(start_menu_metrics_t* m);
static void get_desktop_menu_metrics(desktop_menu_metrics_t* m);
static void raise_runtime_error(const char* title, const char* message);
static void launch_application_exe(const char* exe_name, int fallback_x, int fallback_y);
static void launch_settings_tab(int tab_idx);
static void fill_chamfer_rect(int x, int y, int w, int h, uint8_t color);
static void draw_chamfer_rect(int x, int y, int w, int h, uint8_t color);
static void notepad_load_from_storage(void);
static void settings_md_reset(void);
static void settings_md_add_line(const char* src, uint8_t color, uint8_t indent);
static void settings_md_parse(const char* markdown);
static void settings_md_ensure_loaded(void);
void open_window(int type, int x, int y);

static void explorer_refresh(void) {
    if (explorer_this_pc_view) {
        explorer_entry_count = 0;
        explorer_selected = -1;
        return;
    }
    explorer_entry_count = fs_list_entries(explorer_path, explorer_entries, explorer_is_dir, EXPLORER_MAX_ENTRIES);
    if (explorer_selected >= explorer_entry_count) {
        explorer_selected = -1;
    }
}

static void explorer_open_this_pc(void) {
    explorer_this_pc_view = 1;
    explorer_selected = -1;
    explorer_entry_count = 0;
}

static void explorer_build_this_pc_entries(void) {
    explorer_drive_count = fs_storage_get_device_count();
    if (explorer_drive_count < 1) explorer_drive_count = 1;
    if (explorer_drive_count > EXPLORER_MAX_ENTRIES) explorer_drive_count = EXPLORER_MAX_ENTRIES;

    for (int i = 0; i < explorer_drive_count; i++) {
        explorer_drive_paths[i][0] = (char)('C' + i);
        explorer_drive_paths[i][1] = ':';
        explorer_drive_paths[i][2] = '/';
        explorer_drive_paths[i][3] = 0;

        explorer_drive_labels[i][0] = (char)('C' + i);
        explorer_drive_labels[i][1] = ':';
        explorer_drive_labels[i][2] = ' ';
        explorer_drive_labels[i][3] = 0;
        append_string(explorer_drive_labels[i], sizeof(explorer_drive_labels[i]),
                      (i == 0) ? "System Disk" : "Local Disk");
    }
}

static void ensure_storage_initialized(void) {
    if (storage_initialized) return;
    debug_log_message("Boot: storage init begin");
    debug_present_boot_status();
    initialize_storage_layout();
    storage_initialized = 1;
    settings_md_loaded = 0;
    debug_log_message("Boot: storage init complete");
    debug_present_boot_status();
}

typedef struct {
    const char* label;
    uint32_t width;
    uint32_t height;
    uint8_t truecolor_only;
} display_profile_t;

static const display_profile_t g_display_profiles[] = {
    {"640 x 480 (Safe)", 640, 480, 0},
    {"800 x 600", 800, 600, 1},
    {"1280 x 720", 1280, 720, 1},
    {"Native", 0, 0, 1}
};

static int display_profile_count(void) {
    return (int)(sizeof(g_display_profiles) / sizeof(g_display_profiles[0]));
}

static int display_profile_available(int idx) {
    if (idx < 0 || idx >= display_profile_count()) return 0;
    if (!g_display_profiles[idx].truecolor_only) return 1;
    return graphics_is_truecolor() ? 1 : 0;
}

static void apply_display_profile(void) {
    if (setting_display_profile < 0 || setting_display_profile >= display_profile_count()) {
        setting_display_profile = 0;
    }
    if (!display_profile_available(setting_display_profile)) {
        setting_display_profile = 0;
    }

    const display_profile_t* profile = &g_display_profiles[setting_display_profile];
    uint32_t target_w = profile->width;
    uint32_t target_h = profile->height;
    if (target_w == 0 || target_h == 0) {
        target_w = graphics_get_native_width();
        target_h = graphics_get_native_height();
    }

    if (!graphics_set_resolution(target_w, target_h)) {
        raise_runtime_error("Display Error", "Resolution change failed.");
        return;
    }

    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (!windows[i].active) continue;
        if (windows[i].w > (int)current_width - 8) windows[i].w = (int)current_width - 8;
        if (windows[i].h > (int)current_height - TASKBAR_HEIGHT - 8) windows[i].h = (int)current_height - TASKBAR_HEIGHT - 8;
        if (windows[i].w < WINDOW_MIN_W_DEFAULT) windows[i].w = WINDOW_MIN_W_DEFAULT;
        if (windows[i].h < WINDOW_MIN_H_DEFAULT) windows[i].h = WINDOW_MIN_H_DEFAULT;
        if (windows[i].x > (int)current_width - windows[i].w) windows[i].x = (int)current_width - windows[i].w;
        if (windows[i].y > (int)current_height - TASKBAR_HEIGHT - windows[i].h) {
            windows[i].y = (int)current_height - TASKBAR_HEIGHT - windows[i].h;
        }
        if (windows[i].x < 0) windows[i].x = 0;
        if (windows[i].y < 0) windows[i].y = 0;
    }
}

static const char* path_basename(const char* path) {
    const char* last = path;
    for (const char* p = path; *p; p++) {
        if (*p == '/') {
            last = p + 1;
        }
    }
    if (*last == 0 && last > path) {
        const char* p = last - 1;
        while (p > path && *p != '/') {
            p--;
        }
        return (*p == '/') ? p + 1 : p;
    }
    return last;
}

static int path_has_extension(const char* path, const char* ext) {
    if (!path || !ext) return 0;
    size_t path_len = strlen(path);
    size_t ext_len = strlen(ext);
    if (path_len < ext_len) return 0;
    return strcmp(path + (path_len - ext_len), ext) == 0;
}

static file_t* fs_open_or_create_file(const char* path) {
    file_t* file = fs_open_file(path);
    if (file) return file;
    return fs_create_file(path);
}

static void write_text_file(const char* path, const char* txt) {
    if (!path || !txt) return;
    file_t* file = fs_open_or_create_file(path);
    if (!file) return;
    fs_write_file(file, (const uint8_t*)txt, (uint32_t)strlen(txt));
}

static void explorer_go_parent(void) {
    if (strcmp(explorer_path, "C:/") == 0) return;
    size_t len = strlen(explorer_path);
    while (len > 0 && explorer_path[len - 1] == '/') {
        explorer_path[len - 1] = 0;
        len--;
    }
    while (len > 0 && explorer_path[len - 1] != '/') {
        explorer_path[len - 1] = 0;
        len--;
    }
    if (len == 0) {
        strncpy(explorer_path, "C:/", sizeof(explorer_path) - 1);
        explorer_path[sizeof(explorer_path) - 1] = 0;
    }
    explorer_refresh();
}

static void initialize_storage_layout(void) {
    debug_log_message("Boot: fs_init");
    debug_present_boot_status();
    fs_init();
    debug_log_message("Boot: process model init");
    debug_present_boot_status();
    process_model_init();
    int drive_count = fs_storage_get_device_count();
    if (drive_count < 1) drive_count = 1;
    if (drive_count > 8) drive_count = 8;
    for (int i = 0; i < drive_count; i++) {
        char drive_root[4];
        drive_root[0] = (char)('C' + i);
        drive_root[1] = ':';
        drive_root[2] = '/';
        drive_root[3] = 0;
        fs_create_directory(drive_root);
    }

    debug_log_message("Boot: create directories");
    debug_present_boot_status();

    fs_create_directory("C:/GamerOS");
    fs_create_directory("C:/GamerOS/System32");
    fs_create_directory("C:/GamerOS/Apps");
    fs_create_directory("C:/GamerOS/Apps/About");
    fs_create_directory("C:/GamerOS/Apps/Explorer");
    fs_create_directory("C:/GamerOS/Apps/Notepad");
    fs_create_directory("C:/GamerOS/Apps/Settings");
    fs_create_directory("C:/GamerOS/Apps/BuiltIn");
    fs_create_directory("C:/GamerOS/Apps/Manifests");
    fs_create_directory("C:/GamerOS/Registry");
    fs_create_directory("C:/Users");
    fs_create_directory("C:/Users/Admin");
    fs_create_directory("C:/Users/Admin/AppData");
    fs_create_directory("C:/Users/Admin/AppData/About");
    fs_create_directory("C:/Users/Admin/AppData/Explorer");
    fs_create_directory("C:/Users/Admin/AppData/Notepad");
    fs_create_directory("C:/Users/Admin/AppData/Settings");
    fs_create_directory("C:/Users/Admin/Notepad");
    fs_create_directory("C:/Users/Admin/Settings");
    fs_create_directory("C:/Users/Admin/Explorer");
    fs_create_directory("C:/Users/Admin/Saves");
    fs_create_directory("C:/Users/Admin/Documents");
    fs_create_directory("C:/Users/Admin/Downloads");
    fs_create_directory("C:/GamerOS/Logs");

    debug_log_message("Boot: seed system files");
    debug_present_boot_status();
    write_text_file("C:/GamerOS/System32/GAMEROS.INI",
        "shell=GamerOS\nbuild=1.400\nsystem=C:/GamerOS/System32\napps=C:/GamerOS/Apps\nregistry=C:/GamerOS/Registry\n");
    write_text_file("C:/Users/Admin/README.TXT", "Welcome to GamerOS user profile.\n");
    write_text_file(NOTEPAD_FILE_PATH, "GamerOS Notepad\n\nType here...\n");

    write_text_file("C:/GamerOS/System32/NOTEPAD.EXE", "GOSAPP\nName=Notepad\nEntry=WIN_NOTEPAD\n");
    write_text_file("C:/GamerOS/System32/SETTINGS.EXE", "GOSAPP\nName=Settings\nEntry=WIN_SETTINGS\n");
    write_text_file("C:/GamerOS/System32/EXPLORER.EXE", "GOSAPP\nName=Explorer\nEntry=WIN_EXPLORER\n");
    write_text_file("C:/GamerOS/System32/ABOUT.EXE", "GOSAPP\nName=About GamerOS\nEntry=WIN_ABOUT\n");
    write_text_file("C:/GamerOS/System32/KERNEL.SYS", "Kernel image placeholder\n");
    write_text_file("C:/GamerOS/System32/SHELL32.DLL", "Shell library placeholder\n");

    write_text_file("C:/GamerOS/Apps/BuiltIn/APPS.LST",
        "NOTEPAD.EXE\nSETTINGS.EXE\nEXPLORER.EXE\nABOUT.EXE\n");
    write_text_file("C:/GamerOS/Registry/INSTALLED_APPS.REG",
        "NOTEPAD=Installed\nSETTINGS=Installed\nEXPLORER=Installed\nABOUT=Installed\n");
    write_text_file("C:/GamerOS/Registry/STORAGE.REG",
        "SystemRoot=C:/GamerOS\nSystem32=C:/GamerOS/System32\nAppsRoot=C:/GamerOS/Apps\nUserRoot=C:/Users/Admin\n");

    write_text_file("C:/GamerOS/Apps/Manifests/Notepad.gosapp",
        "Id=NOTEPAD\nDisplayName=Notepad\nExe=C:/GamerOS/System32/NOTEPAD.EXE\nEntry=WIN_NOTEPAD\n"
        "InstallRoot=C:/GamerOS/Apps/Notepad\nUserDataRoot=C:/Users/Admin/AppData/Notepad\nCategory=Utility\nPermissions=filesystem:user-data\n");
    write_text_file("C:/GamerOS/Apps/Manifests/Settings.gosapp",
        "Id=SETTINGS\nDisplayName=Settings\nExe=C:/GamerOS/System32/SETTINGS.EXE\nEntry=WIN_SETTINGS\n"
        "InstallRoot=C:/GamerOS/Apps/Settings\nUserDataRoot=C:/Users/Admin/AppData/Settings\nCategory=System\nPermissions=filesystem:system-info\n");
    write_text_file("C:/GamerOS/Apps/Manifests/Explorer.gosapp",
        "Id=EXPLORER\nDisplayName=File Explorer\nExe=C:/GamerOS/System32/EXPLORER.EXE\nEntry=WIN_EXPLORER\n"
        "InstallRoot=C:/GamerOS/Apps/Explorer\nUserDataRoot=C:/Users/Admin/AppData/Explorer\nCategory=System\nPermissions=filesystem:all-volumes\n");
    write_text_file("C:/GamerOS/Apps/Manifests/About.gosapp",
        "Id=ABOUT\nDisplayName=About GamerOS\nExe=C:/GamerOS/System32/ABOUT.EXE\nEntry=WIN_ABOUT\n"
        "InstallRoot=C:/GamerOS/Apps/About\nUserDataRoot=C:/Users/Admin/AppData/About\nCategory=System\nPermissions=filesystem:system-info\n");

    write_text_file("C:/GamerOS/Apps/Notepad/README.TXT", "Notepad install root.\n");
    write_text_file("C:/GamerOS/Apps/Settings/README.TXT", "Settings install root.\n");
    write_text_file("C:/GamerOS/Apps/Explorer/README.TXT", "Explorer install root.\n");
    write_text_file("C:/GamerOS/Apps/About/README.TXT", "About install root.\n");
    write_text_file("C:/Users/Admin/AppData/Notepad/PREFS.INI", "font=System\nwordwrap=0\n");
    write_text_file("C:/Users/Admin/AppData/Settings/PREFS.INI", "theme=GamerOS Modern\nshow_seconds=0\n");
    write_text_file("C:/Users/Admin/AppData/Explorer/PREFS.INI", "start_path=C:/\nview=details\n");
    write_text_file("C:/Users/Admin/AppData/About/PREFS.INI", "show_diagnostics=1\n");
    write_text_file("C:/Users/Admin/Documents/WELCOME.TXT", "Documents folder ready.\n");
    write_text_file("C:/Users/Admin/Downloads/README.TXT", "Downloads folder ready.\n");
    write_text_file("C:/Users/Admin/Saves/SAVE1.GOS", "Empty save slot 1\n");
    write_text_file("C:/Users/Admin/Saves/SAVE2.GOS", "Empty save slot 2\n");
    write_text_file(settings_ui_changelog_md_path(), settings_ui_changelog_md_text());
    debug_log_message("Boot: storage layout ready");
    debug_present_boot_status();
    debug_log_message("Boot: build explorer drives");
    debug_present_boot_status();
    explorer_build_this_pc_entries();
    debug_log_message("Boot: open explorer root");
    debug_present_boot_status();
    explorer_open_this_pc();
    debug_log_message("Boot: storage post-init done");
    debug_present_boot_status();
}

static void shutdown_os(void) {
    shutdown_requested = 1;
    clear_screen(XP_BLACK);
    draw_string(84, 92, "Shutting down...", XP_WHITE);
    swap_buffers();

    // Try common ACPI/APM poweroff ports used by VMs.
    outw(0x604, 0x2000); // QEMU/ACPI style
    outw(0xB004, 0x2000); // Bochs/QEMU legacy
    outw(0x4004, 0x3400); // VirtualBox legacy
    outw(0x0604, 0x2000); // Alternate ACPI port forms
}

static void draw_compact_string(int x, int y, const char* str, uint8_t color) {
    while (*str) {
        uint8_t uc = (uint8_t)(*str);
        if (uc < 32 || uc > 127) uc = (uint8_t)' ';
        const uint8_t* bitmap = font_8x8[uc - 32];
        for (int row = 0; row < 8; row++) {
            uint8_t row_data = bitmap[row];
            for (int col = 0; col < 6; col++) {
                if (row_data & (0x80 >> col)) {
                    draw_pixel(x + col, y + row, color);
                }
            }
        }
        x += 6;
        str++;
    }
}

static void draw_compact_string_clipped(int x, int y, int max_chars, const char* str, uint8_t color) {
    if (max_chars <= 0) return;
    int i = 0;
    while (str[i] && i < max_chars) {
        char ch[2];
        ch[0] = str[i];
        ch[1] = 0;
        draw_compact_string(x + (i * 6), y, ch, color);
        i++;
    }
}

static void fill_chamfer_rect(int x, int y, int w, int h, uint8_t color) {
    if (w <= 0 || h <= 0) return;
    if (w < 3 || h < 3) {
        fill_rect(x, y, w, h, color);
        return;
    }
    fill_rect(x + 1, y, w - 2, h, color);
    fill_rect(x, y + 1, w, h - 2, color);
}

static void draw_chamfer_rect(int x, int y, int w, int h, uint8_t color) {
    if (w <= 0 || h <= 0) return;
    if (w < 3 || h < 3) {
        draw_rect(x, y, w, h, color);
        return;
    }
    draw_rect(x + 1, y, w - 2, h, color);
    draw_rect(x, y + 1, w, h - 2, color);
}

static void draw_desktop_watermark(void) {
    const char* line1 = "GamerOS 00m1";
    const char* line2 = "Modern shell preview  Build 1.400";
    int w1 = (int)strlen(line1) * 6;
    int w2 = (int)strlen(line2) * 6;
    int max_w = (w1 > w2) ? w1 : w2;
    int card_w = max_w + 18;
    int card_h = 24;
    int x = (int)current_width - card_w - 10;
    int y = (int)current_height - TASKBAR_HEIGHT - card_h - 10;
    if (x < 6) x = 6;
    if (y < 6) y = 6;
    fill_chamfer_rect(x, y, card_w, card_h, XP_WHITE);
    draw_chamfer_rect(x, y, card_w, card_h, XP_DGRAY);
    fill_rect(x + 1, y + 1, 3, card_h - 2, XP_LBLUE);
    draw_compact_string(x + 10, y + 5, line1, XP_BLACK);
    draw_compact_string(x + 10, y + 13, line2, XP_DGRAY);
}

static int wallpaper_is_too_dark(void) {
    static int cached = -1;
    if (cached >= 0) return cached;
    if (background_wallpaper_width == 0 || background_wallpaper_height == 0) {
        cached = 1;
        return cached;
    }

    uint32_t step_x = background_wallpaper_width / 32;
    uint32_t step_y = background_wallpaper_height / 24;
    if (step_x == 0) step_x = 1;
    if (step_y == 0) step_y = 1;

    uint32_t sum = 0;
    uint32_t count = 0;
    for (uint32_t y = 0; y < background_wallpaper_height; y += step_y) {
        uint32_t row = y * background_wallpaper_width;
        for (uint32_t x = 0; x < background_wallpaper_width; x += step_x) {
            sum += (uint32_t)(background_wallpaper_pixels[row + x] & 0x0F);
            count++;
        }
    }
    if (count == 0) {
        cached = 1;
        return cached;
    }
    cached = ((sum / count) <= 2U) ? 1 : 0;
    return cached;
}

static void draw_desktop_wallpaper(int desktop_h) {
    if (desktop_h <= 0) return;
    if (graphics_is_truecolor()) {
        // Bright layered backdrop with geometric panels so the desktop feels
        // intentional even before custom wallpapers arrive.
        for (int y = 0; y < desktop_h; y++) {
            uint32_t t = (uint32_t)((y * 255) / ((desktop_h > 1) ? (desktop_h - 1) : 1));
            uint32_t r = 230 - ((18 * t) / 255);
            uint32_t g = 242 - ((12 * t) / 255);
            uint32_t b = 255;
            uint32_t rgb = (r << 16) | (g << 8) | b;
            for (uint32_t x = 0; x < current_width; x++) {
                draw_pixel_rgb((int)x, y, rgb);
            }
        }

        int panel1_x = (int)current_width / 2 - 40;
        int panel1_y = desktop_h / 4;
        int panel1_w = (int)current_width / 3;
        int panel1_h = desktop_h / 2;
        int panel2_x = panel1_x + panel1_w / 2;
        int panel2_y = panel1_y + panel1_h / 3;
        int panel2_w = (int)current_width / 2;
        int panel2_h = desktop_h / 2;
        if (panel1_x < 0) panel1_x = 0;
        if (panel2_x < 0) panel2_x = 0;

        for (int y = panel1_y; y < panel1_y + panel1_h && y < desktop_h; y++) {
            for (int x = panel1_x; x < panel1_x + panel1_w && x < (int)current_width; x++) {
                if (((x + y) & 1) == 0) draw_pixel_rgb(x, y, 0xDDEBFF);
            }
        }
        for (int y = panel2_y; y < panel2_y + panel2_h && y < desktop_h; y++) {
            for (int x = panel2_x; x < panel2_x + panel2_w && x < (int)current_width; x++) {
                if (((x + y) & 1) == 0) draw_pixel_rgb(x, y, 0xE8F2FF);
            }
        }

        if (!wallpaper_is_too_dark()) {
            uint32_t src_w = background_wallpaper_width;
            uint32_t src_h = background_wallpaper_height;
            uint32_t dst_w = current_width;
            uint32_t dst_h = (uint32_t)desktop_h;
            for (uint32_t y = 0; y < dst_h; y++) {
                uint32_t sy = (y * src_h) / dst_h;
                uint32_t row = sy * src_w;
                for (uint32_t x = 0; x < dst_w; x++) {
                    uint32_t sx = (x * src_w) / dst_w;
                    uint8_t color = background_wallpaper_pixels[row + sx] & 0x0F;
                    if ((color & 0x0F) > 2 && ((x + y) % 5 == 0)) {
                        draw_pixel((int)x, (int)y, color);
                    }
                }
            }
        }
        return;
    }

    if (background_wallpaper_width == 0 || background_wallpaper_height == 0 || wallpaper_is_too_dark()) {
        for (int y = 0; y < desktop_h; y++) {
            uint8_t c = XP_WHITE;
            if (y > (desktop_h * 2) / 3) c = XP_LGRAY;
            else if (y > desktop_h / 3) c = ((y & 1) == 0) ? XP_LGRAY : XP_WHITE;
            fill_rect(0, y, (int)current_width, 1, c);
        }
        return;
    }

    uint32_t src_w = background_wallpaper_width;
    uint32_t src_h = background_wallpaper_height;
    uint32_t dst_w = current_width;
    uint32_t dst_h = (uint32_t)desktop_h;

    for (uint32_t y = 0; y < dst_h; y++) {
        uint32_t sy = (y * src_h) / dst_h;
        uint32_t row = sy * src_w;
        for (uint32_t x = 0; x < dst_w; x++) {
            uint32_t sx = (x * src_w) / dst_w;
            uint8_t color = background_wallpaper_pixels[row + sx] & 0x0F;
            draw_pixel((int)x, (int)y, color);
        }
    }
}

static void append_string(char* dest, size_t dest_cap, const char* src) {
    if (!dest || !src || dest_cap == 0) return;
    size_t dlen = strlen(dest);
    size_t i = 0;
    while (src[i] && dlen + 1 < dest_cap) {
        dest[dlen++] = src[i++];
    }
    dest[dlen] = 0;
}

static void get_window_min_size(const window_t* win, int* out_w, int* out_h) {
    int min_w = WINDOW_MIN_W_DEFAULT;
    int min_h = WINDOW_MIN_H_DEFAULT;
    if (win) {
        if (win->type == WIN_NOTEPAD) {
            min_w = 220;
            min_h = 130;
        } else if (win->type == WIN_SETTINGS || win->type == WIN_EXPLORER) {
            min_w = 240;
            min_h = 150;
        } else if (win->type == WIN_MYCOMP || win->type == WIN_ABOUT) {
            min_w = 180;
            min_h = 120;
        }
    }
    if (out_w) *out_w = min_w;
    if (out_h) *out_h = min_h;
}

static int get_ui_scale(void) {
    if (current_width >= 1200 || current_height >= 720) return 3;
    if (current_width >= 800 || current_height >= 600) return 2;
    return 1;
}

static void get_start_menu_metrics(start_menu_metrics_t* m) {
    if (!m) return;
    int menu_w = START_MENU_W;
    int menu_h = START_MENU_H;
    int item_h = START_MENU_ITEM_H;
    if (menu_w > (int)current_width - 8) menu_w = (int)current_width - 8;
    if (menu_h > (int)current_height - TASKBAR_HEIGHT - 4) menu_h = (int)current_height - TASKBAR_HEIGHT - 4;
    m->x = START_MENU_X;
    m->w = menu_w;
    m->h = menu_h;
    m->item_h = item_h;
    m->y = (int)current_height - TASKBAR_HEIGHT - menu_h;
}

static void get_desktop_menu_metrics(desktop_menu_metrics_t* m) {
    if (!m) return;
    int menu_w = 124;
    int item_h = 16;
    int item_count = 3;
    int menu_h = item_h * item_count + 8;
    int menu_x = desktop_menu_x;
    int menu_y = desktop_menu_y;

    if (menu_x + menu_w > (int)current_width) menu_x = (int)current_width - menu_w;
    if (menu_y + menu_h > (int)current_height - TASKBAR_HEIGHT) menu_y = (int)current_height - TASKBAR_HEIGHT - menu_h;
    if (menu_x < 0) menu_x = 0;
    if (menu_y < 0) menu_y = 0;

    m->x = menu_x;
    m->y = menu_y;
    m->w = menu_w;
    m->h = menu_h;
    m->item_h = item_h;
}

static void raise_runtime_error(const char* title, const char* message) {
    if (!title || !title[0] || !message || !message[0]) return;
    strncpy(error_popup_title, title, sizeof(error_popup_title) - 1);
    error_popup_title[sizeof(error_popup_title) - 1] = 0;
    strncpy(error_popup_message, message, sizeof(error_popup_message) - 1);
    error_popup_message[sizeof(error_popup_message) - 1] = 0;
    error_popup_open = 1;
    serial_write_string("Runtime error: ");
    serial_write_string(error_popup_title);
    serial_write_string(" - ");
    serial_write_string(error_popup_message);
    serial_write_string("\n");

    {
        char line[DEBUG_LOG_CHARS];
        strncpy(line, title, sizeof(line) - 1);
        line[sizeof(line) - 1] = 0;
        append_string(line, sizeof(line), ": ");
        append_string(line, sizeof(line), message);
        debug_log_message(line);
    }
}

static void debug_log_message(const char* message) {
    if (!message || !message[0]) return;

    int slot = debug_log_count;
    if (slot >= DEBUG_LOG_LINES) {
        for (int i = 1; i < DEBUG_LOG_LINES; i++) {
            memcpy(debug_log_lines[i - 1], debug_log_lines[i], DEBUG_LOG_CHARS);
        }
        slot = DEBUG_LOG_LINES - 1;
    } else {
        debug_log_count++;
    }

    strncpy(debug_log_lines[slot], message, DEBUG_LOG_CHARS - 1);
    debug_log_lines[slot][DEBUG_LOG_CHARS - 1] = 0;
    serial_write_string("Debug: ");
    serial_write_string(debug_log_lines[slot]);
    serial_write_string("\n");
}

static void get_debug_overlay_rect(int* out_x, int* out_y, int* out_w, int* out_h) {
    int min_w = 160;
    int min_h = 42;
    int max_w = (int)current_width - 8;
    int max_h = (int)current_height - TASKBAR_HEIGHT - 8;
    int wanted_h = 16 + (debug_log_count * 8);

    if (debug_overlay_w < min_w) debug_overlay_w = min_w;
    if (debug_overlay_h < min_h) debug_overlay_h = min_h;
    if (debug_overlay_h < wanted_h) debug_overlay_h = wanted_h;
    if (debug_overlay_w > max_w) debug_overlay_w = max_w;
    if (debug_overlay_h > max_h) debug_overlay_h = max_h;

    if (debug_overlay_x < 0) debug_overlay_x = 0;
    if (debug_overlay_y < 0) debug_overlay_y = 0;
    if (debug_overlay_x > (int)current_width - debug_overlay_w) {
        debug_overlay_x = (int)current_width - debug_overlay_w;
    }
    if (debug_overlay_y > (int)current_height - TASKBAR_HEIGHT - debug_overlay_h) {
        debug_overlay_y = (int)current_height - TASKBAR_HEIGHT - debug_overlay_h;
    }
    if (debug_overlay_x < 0) debug_overlay_x = 0;
    if (debug_overlay_y < 0) debug_overlay_y = 0;

    if (out_x) *out_x = debug_overlay_x;
    if (out_y) *out_y = debug_overlay_y;
    if (out_w) *out_w = debug_overlay_w;
    if (out_h) *out_h = debug_overlay_h;
}

static void draw_debug_overlay(void) {
    if (!debug_overlay_enabled || debug_log_count <= 0) return;

    int box_x = 0, box_y = 0, box_w = 0, box_h = 0;
    get_debug_overlay_rect(&box_x, &box_y, &box_w, &box_h);
    int visible_rows = (box_h - 18) / 8;
    int start_row = 0;
    if (visible_rows < 1) visible_rows = 1;
    if (debug_log_count > visible_rows) {
        start_row = debug_log_count - visible_rows;
    }

    fill_chamfer_rect(box_x, box_y, box_w, box_h, XP_WHITE);
    draw_chamfer_rect(box_x, box_y, box_w, box_h, XP_DGRAY);
    fill_rect(box_x + 1, box_y + 1, 3, box_h - 2, XP_LBLUE);
    fill_chamfer_rect(box_x + 5, box_y + 1, box_w - 6, 12, XP_LGRAY);
    fill_rect(box_x + 6, box_y + 2, box_w - 8, 1, XP_WHITE);
    draw_compact_string(box_x + 10, box_y + 4, "Debug Console", XP_BLACK);

    for (int i = 0; i < visible_rows; i++) {
        int log_idx = start_row + i;
        if (log_idx < 0 || log_idx >= debug_log_count) break;
        draw_compact_string_clipped(box_x + 10, box_y + 18 + i * 8, (box_w - 16) / 6, debug_log_lines[log_idx], XP_BLACK);
    }

    for (int g = 0; g < 4; g++) {
        int gx = box_x + box_w - 3 - (g * 3);
        int gy = box_y + box_h - 3;
        fill_rect(gx, gy, 2, 2, XP_DGRAY);
    }
}

static void debug_present_boot_status(void) {
    clear_screen(0);
    draw_debug_overlay();
    swap_buffers();
}

static void launch_application_exe(const char* exe_name, int fallback_x, int fallback_y) {
    debug_log_message("App: launch request");
    debug_present_boot_status();
    if (!storage_initialized) {
        ensure_storage_initialized();
    }

    int win_type = WIN_NONE;
    int launch_x = fallback_x;
    int launch_y = fallback_y;
    debug_log_message("App: resolve descriptor");
    debug_present_boot_status();
    const app_descriptor_t* descriptor = apps_find_by_exe(exe_name);
    if (!descriptor) {
        debug_log_message("App: descriptor missing");
        debug_present_boot_status();
        return;
    }
    debug_log_message("App: resolve launch");
    debug_present_boot_status();
    if (!apps_resolve_launch(exe_name, &win_type, &launch_x, &launch_y)) {
        debug_log_message("App: launch mapping failed");
        debug_present_boot_status();
        return;
    }
    if (fallback_x >= 0) launch_x = fallback_x;
    if (fallback_y >= 0) launch_y = fallback_y;

    debug_log_message("App: spawn task");
    debug_present_boot_status();
    int pid = process_spawn_from_exe(descriptor->system_path, win_type);
    if (pid < 0) {
        debug_log_message("App: spawn failed");
        debug_present_boot_status();
        raise_runtime_error("Loader Error", "Failed to load executable task image.");
        return;
    }
    debug_log_message("App: mark running");
    debug_present_boot_status();
    (void)process_mark_running(pid);

    debug_log_message("App: open window");
    debug_present_boot_status();
    open_window(win_type, launch_x, launch_y);
    if (win_type == WIN_NOTEPAD && storage_initialized) {
        debug_log_message("App: load notepad data");
        debug_present_boot_status();
        notepad_load_from_storage();
    }

    debug_log_message("App: launch complete");
    debug_present_boot_status();
    app_lifecycle_mark_launched(descriptor);
}

static void launch_settings_tab(int tab_idx) {
    launch_application_exe("SETTINGS.EXE", -1, -1);
    if (tab_idx >= 0 && tab_idx < SETTINGS_TAB_COUNT) {
        settings_tab = tab_idx;
    }
}

static void settings_md_reset(void) {
    settings_md_line_count = 0;
    settings_md_plan_start = -1;
    settings_update_view = SETTINGS_UPDATE_VIEW_CHANGELOG;
}

static void settings_md_add_line(const char* src, uint8_t color, uint8_t indent) {
    if (!src || settings_md_line_count >= SETTINGS_MD_MAX_LINES) return;
    markdown_line_t* line = &settings_md_lines[settings_md_line_count++];
    size_t i = 0;
    while (src[i] && i + 1 < sizeof(line->text)) {
        if (src[i] != '`') {
            line->text[i] = src[i];
        } else {
            line->text[i] = '\'';
        }
        i++;
    }
    line->text[i] = 0;
    line->color = color;
    line->indent = indent;
}

static void settings_md_parse(const char* markdown) {
    settings_md_reset();
    if (!markdown) return;

    char line_buf[160];
    size_t pos = 0;
    uint8_t in_code = 0;
    for (size_t i = 0;; i++) {
        char ch = markdown[i];
        if (ch == '\r') continue;
        if (ch == '\n' || ch == 0) {
            line_buf[pos] = 0;
            if (strcmp(line_buf, "```") == 0) {
                in_code = (uint8_t)!in_code;
            } else if (in_code) {
                settings_md_add_line(line_buf, XP_DGRAY, 8);
            } else if (strncmp(line_buf, "### ", 4) == 0) {
                settings_md_add_line(line_buf + 4, XP_BLUE, 0);
            } else if (strncmp(line_buf, "## ", 3) == 0) {
                if (strncmp(line_buf + 3, "Planned", 7) == 0 && settings_md_plan_start < 0) {
                    settings_md_plan_start = settings_md_line_count;
                }
                settings_md_add_line(line_buf + 3, XP_BLUE, 0);
            } else if (strncmp(line_buf, "# ", 2) == 0) {
                settings_md_add_line(line_buf + 2, XP_LBLUE, 0);
            } else if (strncmp(line_buf, "  - ", 4) == 0) {
                char nested[160];
                nested[0] = '-';
                nested[1] = ' ';
                nested[2] = 0;
                append_string(nested, sizeof(nested), line_buf + 4);
                settings_md_add_line(nested, XP_BLACK, 12);
            } else if (strncmp(line_buf, "- ", 2) == 0) {
                settings_md_add_line(line_buf, XP_BLACK, 6);
            } else if (line_buf[0] >= '0' && line_buf[0] <= '9' && line_buf[1] == '.' && line_buf[2] == ' ') {
                settings_md_add_line(line_buf, XP_BLACK, 6);
            } else {
                settings_md_add_line(line_buf, XP_BLACK, 0);
            }

            pos = 0;
            if (ch == 0) break;
            continue;
        }
        if (pos + 1 < sizeof(line_buf)) {
            line_buf[pos++] = ch;
        }
    }
}

static void settings_md_ensure_loaded(void) {
    if (settings_md_loaded) return;
    settings_md_loaded = 1;

    if (!storage_initialized) {
        settings_md_parse(settings_ui_changelog_md_text());
        return;
    }

    file_t* md_file = fs_open_file(settings_ui_changelog_md_path());
    if (!md_file || md_file->size == 0) {
        settings_md_parse(settings_ui_changelog_md_text());
        return;
    }

    static uint8_t md_buffer[MAX_FILE_SIZE + 1];
    memset(md_buffer, 0, sizeof(md_buffer));
    fs_read_file(md_file, md_buffer, MAX_FILE_SIZE);
    settings_md_parse((const char*)md_buffer);
}

static void notepad_clear(void) {
    for (int i = 0; i < NOTEPAD_MAX_LINES; i++) {
        for (int j = 0; j < NOTEPAD_MAX_COLS; j++) {
            notepad_lines[i][j] = 0;
        }
    }
    notepad_cursor_x = 0;
    notepad_cursor_y = 0;
    notepad_view_top = 0;
    notepad_dirty = 0;
}

static void notepad_load_from_storage(void) {
    file_t* file = gos_fs_open_file(NOTEPAD_FILE_PATH);
    notepad_clear();
    if (!file || file->size == 0) return;

    memset(notepad_io_buffer, 0, sizeof(notepad_io_buffer));
    fs_read_file(file, notepad_io_buffer, MAX_FILE_SIZE);

    int x = 0;
    int y = 0;
    for (uint32_t i = 0; i < MAX_FILE_SIZE && notepad_io_buffer[i]; i++) {
        char c = (char)notepad_io_buffer[i];
        if (c == '\n') {
            y++;
            x = 0;
            if (y >= NOTEPAD_MAX_LINES) break;
            continue;
        }
        if (c < 32 || c > 126) continue;
        if (x < NOTEPAD_MAX_COLS && y < NOTEPAD_MAX_LINES) {
            notepad_lines[y][x++] = c;
        }
    }
}

static void notepad_save_to_storage(void) {
    file_t* file = gos_fs_open_file(NOTEPAD_FILE_PATH);
    if (!file) file = gos_fs_create_file(NOTEPAD_FILE_PATH);
    if (!file) return;

    memset(notepad_io_buffer, 0, sizeof(notepad_io_buffer));
    uint32_t pos = 0;
    for (int y = 0; y < NOTEPAD_MAX_LINES && pos < MAX_FILE_SIZE - 1; y++) {
        int last = NOTEPAD_MAX_COLS - 1;
        while (last >= 0 && notepad_lines[y][last] == 0) last--;
        for (int x = 0; x <= last && pos < MAX_FILE_SIZE - 1; x++) {
            notepad_io_buffer[pos++] = (uint8_t)notepad_lines[y][x];
        }
        if (y < NOTEPAD_MAX_LINES - 1 && pos < MAX_FILE_SIZE - 1) {
            notepad_io_buffer[pos++] = '\n';
        }
    }
    fs_write_file(file, notepad_io_buffer, pos);
    notepad_dirty = 0;
}

static void startup_animation(void) {
    const char* title = "GamerOS";
    const char* subtitle = "Preparing your workspace...";
    int title_w = (int)strlen(title) * 8;
    int subtitle_w = (int)strlen(subtitle) * 8;
    int panel_w = 340;
    if (panel_w > (int)current_width - 24) panel_w = (int)current_width - 24;
    int panel_h = 132;
    int title_x = ((int)current_width - title_w) / 2;
    int subtitle_x = ((int)current_width - subtitle_w) / 2;
    int title_y = ((int)current_height / 2) - 48;
    int subtitle_y = title_y + 18;
    int panel_x = ((int)current_width - panel_w) / 2;
    int panel_y = title_y - 14;
    int rail_w = 42;
    int panel_inner_x = panel_x + rail_w + 12;
    int panel_inner_w = panel_w - rail_w - 20;
    int loader_cx = (int)current_width / 2;
    int loader_cy = panel_y + panel_h - 34;
    const int loader_r = 18;
    const int loader_dot_r = 3;

    int bg_h = (int)current_height;
    int frames = 68;
    static const int8_t ring_dx[24] = {
         0,  5,  9, 13, 15, 17, 18, 17, 15, 13,  9,  5,
         0, -5, -9,-13,-15,-17,-18,-17,-15,-13, -9, -5
    };
    static const int8_t ring_dy[24] = {
       -18,-17,-15,-12, -9, -5,  0,  5,  9, 12, 15, 17,
        18, 17, 15, 12,  9,  5,  0, -5, -9,-12,-15,-17
    };

    for (int f = 0; f <= frames; f++) {
        // Smoother background: bright gradient where available, dithered fallback otherwise.
        if (graphics_is_truecolor()) {
            for (int y = 0; y < bg_h; y++) {
                uint32_t t = (uint32_t)((y * 255) / ((bg_h > 1) ? (bg_h - 1) : 1));
                uint32_t r = 188 + ((22 * t) / 255);
                uint32_t g = 220 + ((12 * t) / 255);
                uint32_t b = 255;
                uint32_t rgb = (r << 16) | (g << 8) | b;
                for (uint32_t x = 0; x < current_width; x++) {
                    draw_pixel_rgb((int)x, y, rgb);
                }
            }
            // Soft ambient band near lower third.
            int glow_y0 = (bg_h * 2) / 3;
            for (int y = glow_y0; y < bg_h; y++) {
                uint32_t t = (uint32_t)(((y - glow_y0) * 255) / ((bg_h - glow_y0 > 1) ? (bg_h - glow_y0 - 1) : 1));
                uint32_t rgb = ((170 + ((16 * t) / 255)) << 16) | ((205 + ((22 * t) / 255)) << 8) | (255 - ((16 * t) / 255));
                for (uint32_t x = 0; x < current_width; x++) {
                    if (((x + (uint32_t)y) & 1U) == 0U) draw_pixel_rgb((int)x, y, rgb);
                }
            }
        } else {
            for (int y = 0; y < bg_h; y++) {
                uint8_t c = XP_LBLUE;
                if (y > (bg_h * 2) / 3) c = ((y & 1) == 0) ? XP_WHITE : XP_LGRAY;
                else if (y > bg_h / 3) c = ((y & 1) == 0) ? XP_LGRAY : XP_LBLUE;
                fill_rect(0, y, (int)current_width, 1, c);
            }
        }

        // Center card styled to match the Start menu / desktop shell language.
        fill_chamfer_rect(panel_x, panel_y, panel_w, panel_h, XP_WHITE);
        draw_chamfer_rect(panel_x, panel_y, panel_w, panel_h, XP_LGRAY);
        draw_chamfer_rect(panel_x + 1, panel_y + 1, panel_w - 2, panel_h - 2, XP_DGRAY);
        fill_chamfer_rect(panel_x + 4, panel_y + 4, rail_w, panel_h - 8, XP_LGRAY);
        fill_rect(panel_x + 5, panel_y + 5, rail_w - 2, 2, XP_WHITE);
        fill_chamfer_rect(panel_x + 13, panel_y + 12, 24, 14, XP_LBLUE);
        draw_compact_string(panel_x + 21, panel_y + 16, "G", XP_BLACK);

        fill_chamfer_rect(panel_inner_x, panel_y + 8, panel_inner_w, 18, XP_WHITE);
        fill_rect(panel_inner_x + 1, panel_y + 9, panel_inner_w - 2, 2, XP_LBLUE);
        draw_string(title_x, title_y, title, XP_BLACK);

        fill_chamfer_rect(panel_inner_x, panel_y + 32, panel_inner_w, 14, XP_LGRAY);
        draw_chamfer_rect(panel_inner_x, panel_y + 32, panel_inner_w, 14, XP_DGRAY);
        draw_compact_string(panel_inner_x + 6, panel_y + 36, subtitle, XP_BLACK);

        // Circular loading indicator.
        int lit = ((f * 24) / frames);
        if (lit < 1) lit = 1;
        if (lit > 24) lit = 24;
        for (int i = 0; i < 24; i++) {
            int px = loader_cx + ring_dx[i];
            int py = loader_cy + ring_dy[i];
            uint8_t col = (i < lit) ? XP_BLUE : XP_LGRAY;
            if (i == lit - 1) col = XP_LBLUE;
            vga_fill_circle(px, py, loader_dot_r, col);
        }
        draw_compact_string(loader_cx - 24, loader_cy - 3, "Loading", XP_DGRAY);

        swap_buffers();
        int delay_cycles = 900 + ((frames - f) * 6);
        for (volatile int d = 0; d < delay_cycles; d++) {
            __asm__ volatile ("nop");
        }
    }
}

static void bring_window_to_front(int idx) {
    if (idx < 0 || idx >= MAX_WINDOWS || !windows[idx].active) return;
    int target = idx;
    while (target + 1 < MAX_WINDOWS && windows[target + 1].active) {
        target++;
    }
    if (target == idx) {
        active_window = idx;
        return;
    }
    window_t moved = windows[idx];
    for (int i = idx; i < target; i++) {
        windows[i] = windows[i + 1];
    }
    windows[target] = moved;
    active_window = target;
}

// Initialize windows
void init_windows(void) {
    for (int i = 0; i < MAX_WINDOWS; i++) {
        windows[i].active = 0;
    }
    notepad_clear();
}

// Open a window
void open_window(int type, int x, int y) {
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (!windows[i].active) {
            windows[i].active = 1;
            windows[i].type = type;
            windows[i].x = x;
            windows[i].y = y;
            windows[i].dragging = 0;
            windows[i].resizing = 0;
            windows[i].w = 200;
            windows[i].h = 150;
            
            if (type == WIN_NOTEPAD) {
                strncpy(windows[i].title, notepad_ui_title(), 31);
                windows[i].w = (int)current_width - 20;
                windows[i].h = (int)current_height - TASKBAR_HEIGHT - 26;
                windows[i].x = 10;
                windows[i].y = 8;
                notepad_clear();
                notepad_view_top = 0;
            } else if (type == WIN_ABOUT) {
                strncpy(windows[i].title, "About GamerOS", 31);
                windows[i].w = 210;
                windows[i].h = 152;
            } else if (type == WIN_SETTINGS) {
                strncpy(windows[i].title, settings_ui_panel_title(), 31);
                windows[i].w = (int)current_width - 16;
                windows[i].h = (int)current_height - TASKBAR_HEIGHT - 22;
                windows[i].x = 8;
                windows[i].y = 6;
                for (int si = 0; si < SETTINGS_TAB_COUNT; si++) settings_scroll_top[si] = 0;
            } else if (type == WIN_EXPLORER) {
                strncpy(windows[i].title, "File Explorer", 31);
                windows[i].w = (int)current_width - 16;
                windows[i].h = (int)current_height - TASKBAR_HEIGHT - 22;
                windows[i].x = 8;
                windows[i].y = 6;
                explorer_open_this_pc();
            } else if (type == WIN_MYCOMP) {
                strncpy(windows[i].title, "My Computer", 31);
                windows[i].w = 180;
                windows[i].h = 140;
            }
            windows[i].title[31] = 0;
            if (windows[i].w > (int)current_width) windows[i].w = (int)current_width;
            if (windows[i].h > (int)current_height - TASKBAR_HEIGHT) windows[i].h = (int)current_height - TASKBAR_HEIGHT;
            if (windows[i].x < 0) windows[i].x = 0;
            if (windows[i].y < 0) windows[i].y = 0;
            if (windows[i].x > (int)current_width - windows[i].w) windows[i].x = (int)current_width - windows[i].w;
            if (windows[i].y > (int)current_height - TASKBAR_HEIGHT - windows[i].h) {
                windows[i].y = (int)current_height - TASKBAR_HEIGHT - windows[i].h;
            }
            window_count++;
            bring_window_to_front(i);
            return;
        }
    }
}

// Close window
void close_window(int idx) {
    if (idx >= 0 && idx < MAX_WINDOWS && windows[idx].active) {
        if (windows[idx].type == WIN_NOTEPAD && notepad_dirty && storage_initialized) {
            notepad_save_to_storage();
        }
        // Update simple app lifecycle state when a top-level app window closes.
        app_lifecycle_mark_closed_by_type(windows[idx].type);
        (void)process_terminate_by_window_type(windows[idx].type);
        windows[idx].active = 0;
        window_count--;
        for (int i = MAX_WINDOWS - 1; i >= 0; i--) {
            if (windows[i].active) {
                active_window = i;
                return;
            }
        }
        active_window = -1;
    }
}

// Draw window
void draw_window(window_t* win) {
    int x = win->x;
    int y = win->y;
    int w = win->w;
    int h = win->h;

    // Defensive geometry clamp for VM stability.
    if (w < 24 || h < 24) return;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x >= (int)current_width || y >= (int)current_height) return;
    if (x + w > (int)current_width) w = (int)current_width - x;
    if (y + h > (int)current_height - TASKBAR_HEIGHT) h = (int)current_height - TASKBAR_HEIGHT - y;
    if (w < 24 || h < 24) return;
    
    int is_active_window = (active_window >= 0 && win == &windows[active_window]);
    uint8_t frame_fill = is_active_window ? XP_WHITE : XP_LGRAY;
    uint8_t title_fill = is_active_window ? XP_LBLUE : XP_LGRAY;
    uint8_t title_text = XP_BLACK;
    fill_chamfer_rect(x, y, w, h, frame_fill);
    draw_chamfer_rect(x, y, w, h, XP_WHITE);
    draw_chamfer_rect(x + 1, y + 1, w - 2, h - 2, XP_DGRAY);
    fill_rect(x + 4, y + 4, 3, h - 8, XP_LBLUE);
    fill_chamfer_rect(x + 2, y + 26, w - 4, h - 28, XP_WHITE);
    fill_chamfer_rect(x + 2, y + 2, w - 4, 22, title_fill);
    fill_rect(x + 3, y + 3, w - 6, 2, XP_WHITE);
    fill_rect(x + 8, y + 20, w - 36, 1, is_active_window ? XP_WHITE : XP_DGRAY);
    draw_string(x + 14, y + 8, win->title, title_text);
    
    // Close button (slightly larger for easier clicking)
    fill_chamfer_rect(x + w - 26, y + 4, 20, 16, is_active_window ? XP_WHITE : XP_LGRAY);
    draw_chamfer_rect(x + w - 26, y + 4, 20, 16, XP_DGRAY);
    draw_compact_string(x + w - 19, y + 8, "X", XP_BLACK);
    // Bottom-right resize grip.
    for (int g = 0; g < 4; g++) {
        int gx = x + w - 3 - (g * 3);
        int gy = y + h - 2;
        draw_line(gx, gy, x + w - 2, y + h - 3 - (g * 3), XP_DGRAY);
    }
    
    if (win->type == WIN_NOTEPAD) {
        fill_chamfer_rect(x + 4, y + 26, w - 8, h - 30, XP_WHITE);
        draw_chamfer_rect(x + 4, y + 26, w - 8, h - 30, XP_DGRAY);
        fill_chamfer_rect(x + 8, y + 32, w - 16, 18, XP_LGRAY);
        fill_rect(x + 9, y + 33, w - 18, 2, XP_LBLUE);
        draw_compact_string(x + 12, y + 38, notepad_ui_toolbar_hint(), XP_BLACK);
        fill_chamfer_rect(x + 8, y + h - 22, w - 16, 14, XP_LGRAY);
        fill_rect(x + 9, y + h - 21, w - 18, 1, XP_WHITE);
        int text_top = y + 40;
        int text_bottom = y + h - 26;
        int visible_lines = (text_bottom - text_top) / 5;
        if (visible_lines < 1) visible_lines = 1;
        if (notepad_view_top < 0) notepad_view_top = 0;
        if (notepad_view_top > NOTEPAD_MAX_LINES - visible_lines) {
            notepad_view_top = NOTEPAD_MAX_LINES - visible_lines;
            if (notepad_view_top < 0) notepad_view_top = 0;
        }
        fill_chamfer_rect(x + 8, y + 54, w - 16, h - 82, XP_WHITE);
        draw_chamfer_rect(x + 8, y + 54, w - 16, h - 82, XP_LGRAY);
        int max_chars = (w - 24) / 6;
        if (max_chars < 1) max_chars = 1;
        if (max_chars > NOTEPAD_MAX_COLS) max_chars = NOTEPAD_MAX_COLS;
        for (int row = 0; row < visible_lines; row++) {
            int i = notepad_view_top + row;
            if (i >= NOTEPAD_MAX_LINES) break;
            if (notepad_lines[i][0]) {
                draw_compact_string_clipped(x + 13, text_top + 14 + row * 5, max_chars, notepad_lines[i], XP_BLACK);
            }
        }
        draw_compact_string(x + 12, y + h - 18, notepad_dirty ? notepad_ui_status_modified() : notepad_ui_status_saved(), XP_DGRAY);
        if (win == &windows[active_window]) {
            int cx = x + 13 + notepad_cursor_x * 6;
            int cy = text_top + 14 + (notepad_cursor_y - notepad_view_top) * 5;
            if (cy >= text_top + 14 && cy < text_bottom) {
                fill_rect(cx, cy, 5, 6, XP_BLACK);
            }
        }
    } else if (win->type == WIN_SETTINGS) {
        fill_chamfer_rect(x + 4, y + 28, w - 8, h - 32, XP_WHITE);
        draw_chamfer_rect(x + 4, y + 28, w - 8, h - 32, XP_DGRAY);
        int nav_x = x + 6;
        int nav_y = y + 30;
        int nav_w = 124;
        int nav_h = h - 36;
        fill_chamfer_rect(nav_x, nav_y, nav_w, nav_h, XP_LGRAY);
        draw_chamfer_rect(nav_x, nav_y, nav_w, nav_h, XP_DGRAY);
        fill_chamfer_rect(nav_x + 1, nav_y + 1, nav_w - 2, 18, XP_WHITE);
        fill_rect(nav_x + 2, nav_y + 2, nav_w - 4, 2, XP_LBLUE);
        draw_compact_string(nav_x + 8, nav_y + 7, settings_ui_panel_title(), XP_BLACK);

        int content_x = nav_x + nav_w + 8;
        int content_y = y + 30;
        int content_w = w - (content_x - x) - 8;
        int content_h = h - 36;
        int body_y = content_y + 24;
        fill_chamfer_rect(content_x, content_y, content_w, content_h, XP_WHITE);
        draw_chamfer_rect(content_x, content_y, content_w, content_h, XP_DGRAY);
        fill_chamfer_rect(content_x + 1, content_y + 1, content_w - 2, 20, XP_LGRAY);
        fill_rect(content_x + 2, content_y + 2, content_w - 4, 2, XP_LBLUE);
        draw_compact_string_clipped(content_x + 8, content_y + 7, (content_w - 16) / 6, "Modern Control Center", XP_BLACK);
        int content_chars = (content_w - 12) / 6;
        if (content_chars < 1) content_chars = 1;

        for (int i = 0; i < SETTINGS_TAB_COUNT; i++) {
            int ty = nav_y + 28 + i * 20;
            fill_chamfer_rect(nav_x + 5, ty - 1, nav_w - 10, 16, (settings_tab == i) ? XP_LBLUE : XP_WHITE);
            draw_chamfer_rect(nav_x + 5, ty - 1, nav_w - 10, 16, XP_DGRAY);
            if (settings_tab == i) {
                fill_rect(nav_x + 7, ty + 1, 3, 12, XP_WHITE);
            }
            draw_compact_string_clipped(nav_x + 14, ty + 3, (nav_w - 24) / 6, settings_ui_tab_name(i), XP_BLACK);
        }

        if (settings_tab == SETTINGS_TAB_SYSTEM) {
            char opt1[40];
            char opt2[40];
            char opt3[40];
            strncpy(opt1, "1) Taskbar compact: ", sizeof(opt1) - 1);
            opt1[sizeof(opt1) - 1] = 0;
            append_string(opt1, sizeof(opt1), setting_compact_mode ? "On" : "Off");
            strncpy(opt2, "2) Clock seconds: ", sizeof(opt2) - 1);
            opt2[sizeof(opt2) - 1] = 0;
            append_string(opt2, sizeof(opt2), setting_show_seconds ? "On" : "Off");
            strncpy(opt3, "3) Debug window: ", sizeof(opt3) - 1);
            opt3[sizeof(opt3) - 1] = 0;
            append_string(opt3, sizeof(opt3), debug_overlay_enabled ? "On" : "Off");
            draw_compact_string_clipped(content_x + 6, body_y + 6, content_chars, "System Controls", XP_BLACK);
            draw_compact_string_clipped(content_x + 6, body_y + 22, content_chars, opt1, XP_BLACK);
            draw_compact_string_clipped(content_x + 6, body_y + 34, content_chars, opt2, XP_BLACK);
            draw_compact_string_clipped(content_x + 6, body_y + 46, content_chars, opt3, XP_BLACK);
            draw_compact_string_clipped(content_x + 6, body_y + 70, content_chars, "Display", XP_BLACK);
            {
                char current_line[40];
                char native_line[40];
                char backend_line[48];
                sprintf(current_line, "Current: %ux%u", (unsigned int)current_width, (unsigned int)current_height);
                sprintf(native_line, "Native: %ux%u", (unsigned int)graphics_get_native_width(), (unsigned int)graphics_get_native_height());
                sprintf(backend_line, "Backend: %s", graphics_is_truecolor() ? "Framebuffer" : "VGA Safe");
                draw_compact_string_clipped(content_x + 6, body_y + 84, content_chars, current_line, XP_BLACK);
                draw_compact_string_clipped(content_x + 6, body_y + 96, content_chars, native_line, XP_BLACK);
                draw_compact_string_clipped(content_x + 6, body_y + 108, content_chars, backend_line, XP_BLACK);
                for (int i = 0; i < display_profile_count(); i++) {
                    int row_y = body_y + 124 + i * 14;
                    uint8_t bg = (setting_display_profile == i) ? XP_LBLUE : XP_WHITE;
                    fill_chamfer_rect(content_x + 6, row_y - 2, content_w - 16, 12, bg);
                    draw_chamfer_rect(content_x + 6, row_y - 2, content_w - 16, 12, XP_DGRAY);
                    if (display_profile_available(i)) {
                        draw_compact_string_clipped(content_x + 10, row_y + 1, content_chars - 1, g_display_profiles[i].label, XP_BLACK);
                    } else {
                        char unavailable[48];
                        strncpy(unavailable, g_display_profiles[i].label, sizeof(unavailable) - 1);
                        unavailable[sizeof(unavailable) - 1] = 0;
                        append_string(unavailable, sizeof(unavailable), " (needs framebuffer)");
                        draw_compact_string_clipped(content_x + 10, row_y + 1, content_chars - 1, unavailable, XP_DGRAY);
                    }
                }
            }
            draw_compact_string_clipped(content_x + 6, body_y + 184, content_chars, "Storage devices:", XP_BLACK);
            int sc = fs_storage_get_device_count();
            char storage_line[44];
            strncpy(storage_line, "Detected profiles: ", sizeof(storage_line) - 1);
            storage_line[sizeof(storage_line) - 1] = 0;
            if (sc > 9) sc = 9;
            char c[2];
            c[0] = (char)('0' + sc);
            c[1] = 0;
            append_string(storage_line, sizeof(storage_line), c);
            draw_compact_string_clipped(content_x + 6, body_y + 196, content_chars, storage_line, XP_BLACK);
            draw_compact_string_clipped(content_x + 6, body_y + 208, content_chars, "System Disk", XP_BLACK);
        } else if (settings_tab == SETTINGS_TAB_PERSONALIZATION) {
            char res_line[40];
            char scale_line[40];
            sprintf(res_line, "Desktop: %ux%u", (unsigned int)current_width, (unsigned int)current_height);
            sprintf(scale_line, "UI Scale Profile: x%d", get_ui_scale());
            draw_compact_string_clipped(content_x + 6, body_y + 6, content_chars, "Personalization", XP_BLACK);
            draw_compact_string_clipped(content_x + 6, body_y + 22, content_chars, "Theme: GamerOS Modern", XP_BLACK);
            draw_compact_string_clipped(content_x + 6, body_y + 34, content_chars, res_line, XP_BLACK);
            draw_compact_string_clipped(content_x + 6, body_y + 46, content_chars, scale_line, XP_BLACK);
            draw_compact_string_clipped(content_x + 6, body_y + 58, content_chars, "Color profile: Blue", XP_BLACK);
        } else if (settings_tab == SETTINGS_TAB_BLUETOOTH_DEVICES) {
            draw_compact_string_clipped(content_x + 6, body_y + 6, content_chars, "Bluetooth & devices", XP_BLACK);
            draw_compact_string_clipped(content_x + 6, body_y + 22, content_chars, "Bluetooth: Off", XP_BLACK);
            draw_compact_string_clipped(content_x + 6, body_y + 34, content_chars, "Mouse: PS/2 Compatible", XP_BLACK);
            draw_compact_string_clipped(content_x + 6, body_y + 46, content_chars, "Keyboard: PS/2 Standard", XP_BLACK);
        } else if (settings_tab == SETTINGS_TAB_NETWORK_INTERNET) {
            draw_compact_string_clipped(content_x + 6, body_y + 6, content_chars, "Network & internet", XP_BLACK);
            draw_compact_string_clipped(content_x + 6, body_y + 22, content_chars, "Network stack: Not enabled", XP_BLACK);
            draw_compact_string_clipped(content_x + 6, body_y + 34, content_chars, "Adapter: Virtual profile", XP_BLACK);
        } else if (settings_tab == SETTINGS_TAB_APPS) {
            draw_compact_string_clipped(content_x + 6, body_y + 6, content_chars, "Apps", XP_BLACK);
            draw_compact_string_clipped(content_x + 6, body_y + 22, content_chars, "Built-in executables:", XP_BLACK);
            draw_compact_string_clipped(content_x + 6, body_y + 34, content_chars, "- NOTEPAD.EXE", XP_BLACK);
            draw_compact_string_clipped(content_x + 6, body_y + 46, content_chars, "- SETTINGS.EXE", XP_BLACK);
            draw_compact_string_clipped(content_x + 6, body_y + 58, content_chars, "- EXPLORER.EXE", XP_BLACK);
            draw_compact_string_clipped(content_x + 6, body_y + 70, content_chars, "- ABOUT.EXE", XP_BLACK);
            draw_compact_string_clipped(content_x + 6, body_y + 84, content_chars, "App root: C:/GamerOS/Apps", XP_BLACK);
        } else if (settings_tab == SETTINGS_TAB_ACCOUNTS) {
            draw_compact_string_clipped(content_x + 6, body_y + 6, content_chars, "Accounts", XP_BLACK);
            draw_compact_string_clipped(content_x + 6, body_y + 22, content_chars, "Current user: Admin", XP_BLACK);
            draw_compact_string_clipped(content_x + 6, body_y + 34, content_chars, "Profile: C:/Users/Admin", XP_BLACK);
            draw_compact_string_clipped(content_x + 6, body_y + 46, content_chars, "Mode: Local", XP_BLACK);
        } else if (settings_tab == SETTINGS_TAB_TIME_LANGUAGE) {
            uint8_t hrt = 0, mnt = 0, sct = 0;
            get_time(&hrt, &mnt, &sct);
            char tm[16];
            sprintf(tm, "Time: %u:%u:%u", (unsigned int)hrt, (unsigned int)mnt, (unsigned int)sct);
            draw_compact_string_clipped(content_x + 6, body_y + 6, content_chars, "Time & language", XP_BLACK);
            draw_compact_string_clipped(content_x + 6, body_y + 22, content_chars, tm, XP_BLACK);
            draw_compact_string_clipped(content_x + 6, body_y + 34, content_chars, "Language: English (US)", XP_BLACK);
        } else if (settings_tab == SETTINGS_TAB_GAMING) {
            char gfx_mode_line[44];
            uint8_t bpp = graphics_get_bpp();
            sprintf(gfx_mode_line, "Graphics mode: %s %u-bpp",
                    graphics_is_truecolor() ? "VESA true-color" : "VGA indexed",
                    (unsigned int)bpp);
            draw_compact_string_clipped(content_x + 6, body_y + 6, content_chars, "Gaming", XP_BLACK);
            draw_compact_string_clipped(content_x + 6, body_y + 22, content_chars, gfx_mode_line, XP_BLACK);
            draw_compact_string_clipped(content_x + 6, body_y + 34, content_chars, "Input latency mode: Balanced", XP_BLACK);
        } else if (settings_tab == SETTINGS_TAB_ACCESSIBILITY) {
            draw_compact_string_clipped(content_x + 6, body_y + 6, content_chars, "Accessibility", XP_BLACK);
            draw_compact_string_clipped(content_x + 6, body_y + 22, content_chars, "High contrast: Off", XP_BLACK);
            draw_compact_string_clipped(content_x + 6, body_y + 34, content_chars, "Large text mode: Off", XP_BLACK);
        } else if (settings_tab == SETTINGS_TAB_PRIVACY_SECURITY) {
            draw_compact_string_clipped(content_x + 6, body_y + 6, content_chars, "Privacy & security", XP_BLACK);
            draw_compact_string_clipped(content_x + 6, body_y + 22, content_chars, "Auth model: Enabled", XP_BLACK);
            draw_compact_string_clipped(content_x + 6, body_y + 34, content_chars, "Isolation table: Active", XP_BLACK);
        } else if (settings_tab == SETTINGS_TAB_GAMEROS_UPDATE) {
            settings_md_ensure_loaded();
            int total = settings_md_line_count;
            int row_h = 10;
            int visible = (content_h - 50) / row_h;
            if (visible < 1) visible = 1;
            int start_idx = 0;
            int end_idx = total;
            if (settings_md_plan_start >= 0) {
                if (settings_update_view == SETTINGS_UPDATE_VIEW_CHANGELOG) {
                    end_idx = settings_md_plan_start;
                } else if (settings_update_view == SETTINGS_UPDATE_VIEW_ROADMAP) {
                    start_idx = settings_md_plan_start;
                }
            }
            if (end_idx < start_idx) end_idx = start_idx;
            int view_count = end_idx - start_idx;
            int top = settings_scroll_top[SETTINGS_TAB_GAMEROS_UPDATE];
            if (top < 0) top = 0;
            if (top > view_count - visible) top = view_count - visible;
            if (top < 0) top = 0;
            settings_scroll_top[SETTINGS_TAB_GAMEROS_UPDATE] = top;

            // Sub-buttons for Build changelog vs Roadmap
            int sub_x = content_x + 6;
            int sub_y = content_y + 4;
            int sub_gap = 8;
            int sub_w = (content_w - 12 - sub_gap) / 2;
            int sub_h = 16;
            if (sub_w < 60) sub_w = 60;
            // Build (current build changelog)
            fill_chamfer_rect(sub_x, sub_y, sub_w, sub_h,
                              (settings_update_view == SETTINGS_UPDATE_VIEW_CHANGELOG) ? XP_LBLUE : XP_WHITE);
            draw_chamfer_rect(sub_x, sub_y, sub_w, sub_h, XP_DGRAY);
            draw_compact_string_clipped(sub_x + 4, sub_y + 4, (sub_w - 8) / 6, "Build 1.400 changelog", XP_BLACK);
            // Roadmap
            int sub2_x = sub_x + sub_w + sub_gap;
            fill_chamfer_rect(sub2_x, sub_y, sub_w, sub_h,
                              (settings_update_view == SETTINGS_UPDATE_VIEW_ROADMAP) ? XP_LBLUE : XP_WHITE);
            draw_chamfer_rect(sub2_x, sub_y, sub_w, sub_h, XP_DGRAY);
            draw_compact_string_clipped(sub2_x + 4, sub_y + 4, (sub_w - 8) / 6, "Roadmap", XP_BLACK);

            for (int r = 0; r < visible; r++) {
                int idx = start_idx + top + r;
                if (idx >= end_idx) break;
                markdown_line_t* md = &settings_md_lines[idx];
                int line_x = content_x + 6 + md->indent;
                int line_y = body_y + 22 + r * row_h;

                // Simple markdown-aware layout:
                // - top-level headings ("#"/"##") are rendered with the larger
                //   regular font when indent == 0 and heading colors are used;
                // - all other text uses the compact font with clipping.
                if (md->indent == 0 && (md->color == XP_BLUE || md->color == XP_LBLUE)) {
                    draw_string(line_x, line_y, md->text, md->color);
                } else {
                    draw_compact_string_clipped(line_x, line_y, content_chars, md->text, md->color);
                }
            }
            draw_compact_string_clipped(content_x + 6, content_y + content_h - 10, content_chars, "Markdown viewer: mouse wheel scroll", XP_DGRAY);
        }
    } else if (win->type == WIN_ABOUT) {
        fill_chamfer_rect(x + 4, y + 28, w - 8, h - 32, XP_WHITE);
        draw_chamfer_rect(x + 4, y + 28, w - 8, h - 32, XP_DGRAY);
        fill_chamfer_rect(x + 8, y + 32, w - 16, 22, XP_LGRAY);
        fill_rect(x + 9, y + 33, w - 18, 2, XP_LBLUE);
        draw_string(x + 14, y + 38, "About GamerOS", XP_BLACK);

        int content_x = x + 10;
        int content_y = y + 62;
        int content_w = w - 20;
        int content_chars = content_w / 6;
        if (content_chars < 1) content_chars = 1;
        char gfx_line[64];
        uint8_t bpp = graphics_get_bpp();
        sprintf(gfx_line, "Graphics: %ux%ux%u %s",
                (unsigned int)current_width,
                (unsigned int)current_height,
                (unsigned int)bpp,
                graphics_is_truecolor() ? "RGBA" : "Indexed");
        draw_compact_string_clipped(content_x, content_y + 0, content_chars, "Version: 00m1", XP_BLACK);
        draw_compact_string_clipped(content_x, content_y + 12, content_chars, "Build: 1.400", XP_BLACK);
        draw_compact_string_clipped(content_x, content_y + 24, content_chars, "Design language: GamerOS Modern", XP_BLACK);
        draw_compact_string_clipped(content_x, content_y + 42, content_chars, "System", XP_DGRAY);
        draw_compact_string_clipped(content_x, content_y + 54, content_chars, "Kernel: x86_64", XP_BLACK);
        draw_compact_string_clipped(content_x, content_y + 66, content_chars, gfx_line, XP_BLACK);
        draw_compact_string_clipped(content_x, content_y + 78, content_chars, "System32: C:/GamerOS/System32", XP_BLACK);
        draw_compact_string_clipped(content_x, content_y + 96, content_chars, "Apps", XP_DGRAY);
        draw_compact_string_clipped(content_x, content_y + 108, content_chars, "Apps: *.EXE (NOTEPAD/SETTINGS/EXPLORER/ABOUT)", XP_BLACK);
        draw_compact_string_clipped(content_x, content_y + 120, content_chars, "Author: Chosentechies", XP_BLACK);
    } else if (win->type == WIN_EXPLORER) {
        fill_chamfer_rect(x + 4, y + 26, w - 8, h - 30, XP_LGRAY);
        draw_chamfer_rect(x + 4, y + 26, w - 8, h - 30, XP_DGRAY);

        fill_chamfer_rect(x + 6, y + 28, w - 12, 18, XP_WHITE);
        fill_rect(x + 7, y + 29, w - 14, 2, XP_LBLUE);
        draw_compact_string(x + 12, y + 34, explorer_this_pc_view ? explorer_ui_title() : explorer_path, XP_BLACK);
        if (!explorer_this_pc_view) {
            fill_chamfer_rect(x + w - 58, y + 31, 50, 12, XP_LGRAY);
            draw_chamfer_rect(x + w - 58, y + 31, 50, 12, XP_DGRAY);
            draw_compact_string(x + w - 50, y + 33, "Go Up", XP_BLACK);
        }
        fill_chamfer_rect(x + 8, y + 50, 86, h - 60, XP_WHITE);
        draw_chamfer_rect(x + 8, y + 50, 86, h - 60, XP_DGRAY);
        fill_rect(x + 9, y + 51, 84, 2, XP_LBLUE);
        draw_compact_string(x + 14, y + 56, "Quick Access", XP_BLACK);
        draw_compact_string(x + 14, y + 74, "Desktop", XP_BLACK);
        draw_compact_string(x + 14, y + 88, "Documents", XP_BLACK);
        draw_compact_string(x + 14, y + 102, "Downloads", XP_BLACK);
        draw_compact_string(x + 14, y + 116, "This PC", XP_BLACK);
        fill_chamfer_rect(x + 98, y + 50, w - 104, h - 60, XP_WHITE);
        draw_chamfer_rect(x + 98, y + 50, w - 104, h - 60, XP_DGRAY);
        int list_x = x + 100;
        int list_y = y + 58;
        int list_w = w - 110;
        int max_chars = list_w / 6;
        if (max_chars < 1) max_chars = 1;
        int row_h = 12;

        if (explorer_this_pc_view) {
            for (int i = 0; i < explorer_drive_count; i++) {
                int row_y = list_y + i * row_h;
                fill_chamfer_rect(x + 96, row_y - 1, w - 104, row_h - 1, (explorer_selected == i) ? XP_LBLUE : XP_WHITE);
                draw_compact_string_clipped(list_x, row_y, max_chars, explorer_drive_labels[i], XP_BLACK);
            }
        } else {
            for (int i = 0; i < explorer_entry_count; i++) {
                int row_y = list_y + i * row_h;
                const char* base = path_basename(explorer_entries[i]);
                char line[MAX_FILENAME_LEN + 8];
                line[0] = 0;
                if (explorer_is_dir[i]) {
                    append_string(line, sizeof(line), "[DIR] ");
                } else {
                    append_string(line, sizeof(line), "      ");
                }
                append_string(line, sizeof(line), base);
                fill_chamfer_rect(x + 96, row_y - 1, w - 104, row_h - 1, (explorer_selected == i) ? XP_LBLUE : XP_WHITE);
                draw_compact_string_clipped(list_x, row_y, max_chars, line, XP_BLACK);
            }
            if (explorer_entry_count == 0) {
                draw_compact_string_clipped(list_x, list_y, max_chars, "(empty folder)", XP_DGRAY);
            }
        }
    }
}

// Desktop icons
typedef struct {
    int x, y;
    const char* label;
    const char* exe_name;
    int window_type;
} desktop_icon_t;

static desktop_icon_t desktop_icons[] = {
    {18, 18, "Notepad", "NOTEPAD.EXE", WIN_NOTEPAD},
    {18, 62, "Settings", "SETTINGS.EXE", WIN_SETTINGS},
    {18, 106, "Explorer", "EXPLORER.EXE", WIN_EXPLORER},
};
#define NUM_ICONS (sizeof(desktop_icons) / sizeof(desktop_icons[0]))

static void layout_desktop_icons(void) {
    int scale = get_ui_scale();
    int base_x = 18 + (scale - 1) * 10;
    int base_y = 18 + (scale - 1) * 8;
    int spacing = 44 + (scale - 1) * 16;
    for (int i = 0; i < (int)NUM_ICONS; i++) {
        desktop_icons[i].x = base_x;
        desktop_icons[i].y = base_y + (i * spacing);
    }
}

static void draw_notepad_icon(int x, int y) {
    fill_chamfer_rect(x, y, DESKTOP_ICON_W, DESKTOP_ICON_H, XP_WHITE);
    draw_chamfer_rect(x, y, DESKTOP_ICON_W, DESKTOP_ICON_H, XP_DGRAY);
    fill_rect(x + 2, y + 2, 3, DESKTOP_ICON_H - 4, XP_LBLUE);
    fill_rect(x + DESKTOP_ICON_W - 5, y, 5, 5, XP_LGRAY);
    draw_line(x + DESKTOP_ICON_W - 5, y + 4, x + DESKTOP_ICON_W - 1, y, XP_DGRAY);
    fill_rect(x + 7, y + 6, DESKTOP_ICON_W - 10, 1, XP_DGRAY);
    fill_rect(x + 7, y + 10, DESKTOP_ICON_W - 10, 1, XP_DGRAY);
    fill_rect(x + 7, y + 14, DESKTOP_ICON_W - 10, 1, XP_DGRAY);
}

static void draw_settings_icon(int x, int y) {
    fill_chamfer_rect(x, y, DESKTOP_ICON_W, DESKTOP_ICON_H, XP_WHITE);
    draw_chamfer_rect(x, y, DESKTOP_ICON_W, DESKTOP_ICON_H, XP_DGRAY);
    fill_rect(x + 2, y + 2, 3, DESKTOP_ICON_H - 4, XP_LBLUE);
    fill_rect(x + 8, y + 4, 4, 12, XP_DGRAY);
    fill_rect(x + 4, y + 8, 12, 4, XP_DGRAY);
    fill_rect(x + 6, y + 6, 8, 8, XP_LGRAY);
    fill_rect(x + 8, y + 8, 4, 4, XP_WHITE);
}

static void draw_explorer_icon(int x, int y) {
    fill_chamfer_rect(x, y + 4, DESKTOP_ICON_W, DESKTOP_ICON_H - 4, XP_WHITE);
    draw_chamfer_rect(x, y + 4, DESKTOP_ICON_W, DESKTOP_ICON_H - 4, XP_DGRAY);
    fill_rect(x + 2, y + 6, 3, DESKTOP_ICON_H - 8, XP_LBLUE);
    fill_chamfer_rect(x + 2, y + 1, 10, 6, XP_LGRAY);
    draw_chamfer_rect(x + 2, y + 1, 10, 6, XP_DGRAY);
    fill_rect(x + 6, y + 10, 10, 6, XP_LGRAY);
}

static void draw_desktop_icon(int x, int y, int window_type) {
    if (window_type == WIN_SETTINGS) {
        draw_settings_icon(x, y);
        return;
    } else if (window_type == WIN_EXPLORER) {
        draw_explorer_icon(x, y);
        return;
    }
    draw_notepad_icon(x, y);
}

static void draw_start_menu(void) {
    start_menu_metrics_t sm = {0};
    get_start_menu_metrics(&sm);
    int menu_x = sm.x;
    int menu_w = sm.w;
    int menu_h = sm.h;
    int menu_y = sm.y;
    int item_h = sm.item_h;

    // Modern light panel style (wider, cleaner spacing, larger action rows).
    fill_chamfer_rect(menu_x, menu_y, menu_w, menu_h, XP_WHITE);
    draw_chamfer_rect(menu_x, menu_y, menu_w, menu_h, XP_LGRAY);
    draw_chamfer_rect(menu_x + 1, menu_y + 1, menu_w - 2, menu_h - 2, XP_DGRAY);

    int rail_w = 42;
    int panel_x = menu_x + rail_w + 8;
    int panel_w = menu_w - rail_w - 14;
    int items_top = menu_y + 46;

    // Left profile rail + avatar chip.
    fill_chamfer_rect(menu_x + 4, menu_y + 4, rail_w, menu_h - 8, XP_LGRAY);
    fill_rect(menu_x + 5, menu_y + 5, rail_w - 2, 2, XP_LBLUE);
    fill_chamfer_rect(menu_x + 11, menu_y + 10, 28, 16, XP_WHITE);
    draw_chamfer_rect(menu_x + 11, menu_y + 10, 28, 16, XP_DGRAY);
    draw_compact_string(menu_x + 21, menu_y + 15, "A", XP_BLACK);
    draw_compact_string(menu_x + 13, menu_y + 34, "Admin", XP_BLACK);
    draw_compact_string(menu_x + 9, menu_y + 46, "Profile", XP_DGRAY);

    // Header + search strip.
    fill_chamfer_rect(panel_x, menu_y + 4, panel_w, 20, XP_WHITE);
    fill_rect(panel_x + 1, menu_y + 5, panel_w - 2, 2, XP_LBLUE);
    draw_string(panel_x + 6, menu_y + 10, "GamerOS", XP_BLACK);
    draw_compact_string(panel_x + 74, menu_y + 11, "Launchpad", XP_DGRAY);
    fill_chamfer_rect(panel_x, menu_y + 28, panel_w, 14, XP_LGRAY);
    draw_chamfer_rect(panel_x, menu_y + 28, panel_w, 14, XP_DGRAY);
    draw_compact_string(panel_x + 6, menu_y + 32, "Search apps", XP_BLACK);

    // Menu items
    const char* items[] = {
        "Notepad",
        "Settings",
        "File Explorer",
        "GamerOS Update",
        "About GamerOS"
    };
    int item_count = 5;
    int text_chars = (panel_w - 24) / 6;
    if (text_chars < 1) text_chars = 1;
    for (int i = 0; i < item_count; i++) {
        int iy = items_top + i * item_h;
        fill_chamfer_rect(panel_x, iy, panel_w, item_h - 2, (i == 1) ? XP_LBLUE : XP_WHITE);
        draw_chamfer_rect(panel_x, iy, panel_w, item_h - 2, XP_DGRAY);
        fill_chamfer_rect(panel_x + 6, iy + 4, 10, 10, XP_LGRAY);
        draw_chamfer_rect(panel_x + 6, iy + 4, 10, 10, XP_DGRAY);
        draw_compact_string_clipped(panel_x + 22, iy + 6, text_chars, items[i], XP_BLACK);
    }

    fill_chamfer_rect(panel_x, menu_y + menu_h - 24, panel_w, 16, XP_LGRAY);
    draw_chamfer_rect(panel_x, menu_y + menu_h - 24, panel_w, 16, XP_DGRAY);
    fill_rect(panel_x + 1, menu_y + menu_h - 23, panel_w - 2, 2, XP_LBLUE);
    draw_compact_string(panel_x + 8, menu_y + menu_h - 20, "Shut Down", XP_BLACK);
}

static void draw_desktop_context_menu(void) {
    desktop_menu_metrics_t dm = {0};
    get_desktop_menu_metrics(&dm);
    const char* items[] = {
        "About GamerOS",
        "Settings",
        "Refresh Desktop"
    };
    int item_count = 3;

    fill_chamfer_rect(dm.x, dm.y, dm.w, dm.h, XP_WHITE);
    draw_chamfer_rect(dm.x, dm.y, dm.w, dm.h, XP_DGRAY);
    fill_rect(dm.x + 1, dm.y + 1, dm.w - 2, 2, XP_LBLUE);
    for (int i = 0; i < item_count; i++) {
        int iy = dm.y + 4 + i * dm.item_h;
        fill_chamfer_rect(dm.x + 4, iy, dm.w - 8, dm.item_h - 2, XP_WHITE);
        draw_chamfer_rect(dm.x + 4, iy, dm.w - 8, dm.item_h - 2, XP_DGRAY);
        fill_chamfer_rect(dm.x + 8, iy + 3, 8, 8, XP_LGRAY);
        draw_chamfer_rect(dm.x + 8, iy + 3, 8, 8, XP_DGRAY);
        draw_compact_string(dm.x + 22, iy + 4, items[i], XP_BLACK);
    }
}

static void draw_error_popup(void) {
    if (!error_popup_open) return;
    int w = 264;
    int h = 94;
    int x = ((int)current_width - w) / 2;
    int y = (((int)current_height - TASKBAR_HEIGHT) - h) / 2;
    if (x < 4) x = 4;
    if (y < 4) y = 4;

    fill_chamfer_rect(x, y, w, h, XP_WHITE);
    draw_chamfer_rect(x, y, w, h, XP_DGRAY);
    fill_chamfer_rect(x + 2, y + 2, w - 4, 16, XP_LBLUE);
    draw_compact_string(x + 8, y + 6, error_popup_title, XP_BLACK);

    draw_compact_string_clipped(x + 8, y + 26, (w - 16) / 6, error_popup_message, XP_BLACK);
    draw_compact_string_clipped(x + 8, y + 38, (w - 16) / 6, "Check serial log for details.", XP_DGRAY);

    fill_chamfer_rect(x + w - 58, y + h - 22, 46, 14, XP_WHITE);
    draw_chamfer_rect(x + w - 58, y + h - 22, 46, 14, XP_DGRAY);
    draw_compact_string(x + w - 44, y + h - 18, "OK", XP_BLACK);
}

void draw_desktop(void) {
    layout_desktop_icons();
    int desktop_h = (int)current_height - TASKBAR_HEIGHT;
    draw_desktop_wallpaper(desktop_h);
    draw_desktop_watermark();
    
    for (int i = 0; i < (int)NUM_ICONS; i++) {
        int x = desktop_icons[i].x;
        int y = desktop_icons[i].y;
        draw_desktop_icon(x, y, desktop_icons[i].window_type);
        int label_w = (int)strlen(desktop_icons[i].label) * 6;
        int label_x = x + (DESKTOP_ICON_W / 2) - (label_w / 2);
        fill_chamfer_rect(label_x - 4, y + DESKTOP_ICON_H + 1, label_w + 8, 12, XP_WHITE);
        draw_chamfer_rect(label_x - 4, y + DESKTOP_ICON_H + 1, label_w + 8, 12, XP_DGRAY);
        draw_compact_string(label_x, y + DESKTOP_ICON_H + 3, desktop_icons[i].label, XP_BLACK);
    }
    
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].active) {
            draw_window(&windows[i]);
        }
    }
    
    // Taskbar
    int taskbar_y = (int)current_height - TASKBAR_HEIGHT;
    int start_y = taskbar_y + ((TASKBAR_HEIGHT - START_BTN_H) / 2);
    int taskbtn_y = taskbar_y + ((TASKBAR_HEIGHT - TASKBTN_H) / 2);
    fill_rect(0, taskbar_y, current_width, TASKBAR_HEIGHT, XP_LGRAY);
    fill_rect(0, taskbar_y, current_width, 2, XP_WHITE);
    fill_rect(0, taskbar_y + TASKBAR_HEIGHT - 2, current_width, 2, XP_DGRAY);

    // Start button (center text inside the button using compact font for consistent metrics)
    fill_chamfer_rect(START_BTN_X, start_y, START_BTN_W, START_BTN_H, XP_WHITE);
    fill_rect(START_BTN_X + 1, start_y + 1, START_BTN_W - 2, 2, XP_LBLUE);
    fill_rect(START_BTN_X + 6, start_y + 4, 4, 8, XP_LBLUE);
    draw_chamfer_rect(START_BTN_X, start_y, START_BTN_W, START_BTN_H, XP_DGRAY);
    {
        const char* start_label = "Start";
        int start_text_w = (int)strlen(start_label) * 6;
        int start_text_x = START_BTN_X + 14 + ((START_BTN_W - 14 - start_text_w) / 2);
        int start_text_y = start_y + (START_BTN_H - 8) / 2;
        draw_compact_string(start_text_x, start_text_y, start_label, XP_BLACK);
    }

    if (start_menu_open) {
        draw_start_menu();
    }
    if (desktop_menu_open) {
        draw_desktop_context_menu();
    }
    draw_error_popup();
    draw_debug_overlay();
    
    // Taskbar items
    int taskbtn_w = setting_compact_mode ? TASKBTN_W : (TASKBTN_W + 16);
    int task_x = 56;
    int clock_w = taskbar_clock_width();
    int clock_h = TASKBTN_H;
    int clock_x = (int)current_width - clock_w - 4;
    int clock_y = taskbar_y + ((TASKBAR_HEIGHT - clock_h) / 2);
    int task_right_limit = clock_x - 4;
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].active) {
            if (task_x + taskbtn_w > task_right_limit) {
                break;
            }
            if (i == active_window) {
                fill_chamfer_rect(task_x, taskbtn_y, taskbtn_w, TASKBTN_H, XP_LBLUE);
            } else {
                fill_chamfer_rect(task_x, taskbtn_y, taskbtn_w, TASKBTN_H, XP_WHITE);
            }
            draw_chamfer_rect(task_x, taskbtn_y, taskbtn_w, TASKBTN_H, XP_DGRAY);
            if (i == active_window) {
                fill_rect(task_x + 1, taskbtn_y + 1, taskbtn_w - 2, 2, XP_WHITE);
            }

            char task_text[9];
            size_t len = strlen(windows[i].title);
            if (len > 8) len = 8;
            strncpy(task_text, windows[i].title, len);
            task_text[len] = 0;
            int tx = task_x + (taskbtn_w - ((int)len * 6)) / 2;
            int ty = taskbtn_y + (TASKBTN_H - 8) / 2;
            draw_compact_string(tx, ty, task_text, XP_BLACK);
            task_x += taskbtn_w + 2;
        }
    }
    
    // Clock
    uint8_t hour = 0, minute = 0, second = 0;
    uint8_t day = 0, month = 0, weekday = 0;
    uint16_t year = 0;
    get_time(&hour, &minute, &second);
    get_date(&day, &month, &year, &weekday);
    char datetime_str[20];
    datetime_str[0] = (char)('0' + ((hour / 10) % 10));
    datetime_str[1] = (char)('0' + (hour % 10));
    datetime_str[2] = ':';
    datetime_str[3] = (char)('0' + ((minute / 10) % 10));
    datetime_str[4] = (char)('0' + (minute % 10));
    int datetime_len = 5;
    if (setting_show_seconds) {
        datetime_str[5] = ':';
        datetime_str[6] = (char)('0' + ((second / 10) % 10));
        datetime_str[7] = (char)('0' + (second % 10));
        datetime_len = 8;
    }
    datetime_str[datetime_len++] = ' ';
    datetime_str[datetime_len++] = (char)('0' + ((day / 10) % 10));
    datetime_str[datetime_len++] = (char)('0' + (day % 10));
    datetime_str[datetime_len++] = '/';
    datetime_str[datetime_len++] = (char)('0' + ((month / 10) % 10));
    datetime_str[datetime_len++] = (char)('0' + (month % 10));
    datetime_str[datetime_len++] = '/';
    datetime_str[datetime_len++] = (char)('0' + ((year / 1000) % 10));
    datetime_str[datetime_len++] = (char)('0' + ((year / 100) % 10));
    datetime_str[datetime_len++] = (char)('0' + ((year / 10) % 10));
    datetime_str[datetime_len++] = (char)('0' + (year % 10));
    datetime_str[datetime_len] = 0;
    fill_chamfer_rect(clock_x, clock_y, clock_w, clock_h, XP_WHITE);
    draw_chamfer_rect(clock_x, clock_y, clock_w, clock_h, XP_DGRAY);
    fill_rect(clock_x + 1, clock_y + 1, clock_w - 2, 2, XP_LBLUE);
    {
        int clock_text_w = ((int)strlen(datetime_str)) * 6;
        int clock_text_x = clock_x + (clock_w - clock_text_w) / 2;
        int clock_text_y = clock_y + (clock_h - 8) / 2;
        draw_compact_string(clock_text_x, clock_text_y, datetime_str, XP_BLACK);
    }
}

static int find_top_window_at(int32_t mx, int32_t my) {
    for (int i = MAX_WINDOWS - 1; i >= 0; i--) {
        if (!windows[i].active) continue;
        if (mx >= windows[i].x && mx < windows[i].x + windows[i].w &&
            my >= windows[i].y && my < windows[i].y + windows[i].h) {
            return i;
        }
    }
    return -1;
}

static int handle_settings_click(window_t* win, int32_t mx, int32_t my) {
    int content_x = win->x + 4;
    int content_y = win->y + 28;
    int content_w = win->w - 8;
    int content_h = win->h - 32;
    if (mx < content_x || mx >= content_x + content_w || my < content_y || my >= content_y + content_h) {
        return 0;
    }

    int nav_x = win->x + 6;
    int nav_y = win->y + 30;
    int nav_w = 124;
    int nav_row_h = 20;
    int nav_start_y = nav_y + 22;
    if (mx >= nav_x + 3 && mx < nav_x + nav_w - 3 &&
        my >= nav_start_y && my < nav_start_y + SETTINGS_TAB_COUNT * nav_row_h) {
        int tab_idx = (my - nav_start_y) / nav_row_h;
        if (tab_idx >= 0 && tab_idx < SETTINGS_TAB_COUNT) {
            settings_tab = tab_idx;
            return 1;
        }
    }

    // Sub-buttons inside GamerOS Update tab.
    if (settings_tab == SETTINGS_TAB_GAMEROS_UPDATE) {
        int sub_x = nav_x + nav_w + 8 + 6;
        int sub_y = win->y + 30 + 4;
        int content_w = win->w - ((sub_x - 6) - win->x) - 8;
        int sub_gap = 8;
        int sub_w = (content_w - 12 - sub_gap) / 2;
        int sub_h = 16;
        if (sub_w < 60) sub_w = 60;
        // Build changelog button
        if (mx >= sub_x && mx < sub_x + sub_w &&
            my >= sub_y && my < sub_y + sub_h) {
            settings_update_view = SETTINGS_UPDATE_VIEW_CHANGELOG;
            settings_scroll_top[SETTINGS_TAB_GAMEROS_UPDATE] = 0;
            return 1;
        }
        // Roadmap button
        int sub2_x = sub_x + sub_w + sub_gap;
        if (mx >= sub2_x && mx < sub2_x + sub_w &&
            my >= sub_y && my < sub_y + sub_h) {
            settings_update_view = SETTINGS_UPDATE_VIEW_ROADMAP;
            settings_scroll_top[SETTINGS_TAB_GAMEROS_UPDATE] = 0;
            return 1;
        }
    }

    if (settings_tab == SETTINGS_TAB_SYSTEM) {
        int content_x = nav_x + nav_w + 6;
        int content_y = win->y + 30;
        int body_y = content_y + 24;
        int rel_x = mx - (content_x + 6);
        int rel_y = my - (body_y + 22);
        if (rel_x >= 0 && rel_x < 150) {
            if (rel_y >= 0 && rel_y < 8) {
                setting_compact_mode = (uint8_t)!setting_compact_mode;
                return 1;
            }
            if (rel_y >= 12 && rel_y < 20) {
                setting_show_seconds = (uint8_t)!setting_show_seconds;
                return 1;
            }
            if (rel_y >= 24 && rel_y < 32) {
                debug_overlay_enabled = (uint8_t)!debug_overlay_enabled;
                return 1;
            }
        }
    }
    if (settings_tab == SETTINGS_TAB_SYSTEM) {
        int content_x = nav_x + nav_w + 6;
        int content_y = win->y + 30;
        int body_y = content_y + 24;
        int list_x = content_x + 6;
        int list_w = win->w - (list_x - win->x) - 14;
        if (mx >= list_x && mx < list_x + list_w) {
            for (int i = 0; i < display_profile_count(); i++) {
                int row_y = body_y + 124 + i * 14;
                if (my >= row_y - 2 && my < row_y + 10) {
                    if (!display_profile_available(i)) {
                        raise_runtime_error("Display Error", "This profile needs framebuffer mode.");
                        return 1;
                    }
                    setting_display_profile = i;
                    apply_display_profile();
                    return 1;
                }
            }
        }
    }
    if (settings_tab == SETTINGS_TAB_SYSTEM) {
        int content_x = nav_x + nav_w + 6;
        int content_y = win->y + 30;
        int body_y = content_y + 24;
        int text_x0 = content_x + 6;
        int text_x1 = content_x + win->w - (content_x - win->x) - 8;
        if (mx >= text_x0 && mx < text_x1) {
            if (my >= body_y + 22 && my < body_y + 30) { setting_compact_mode = !setting_compact_mode; return 1; }
            if (my >= body_y + 34 && my < body_y + 42) { setting_show_seconds = !setting_show_seconds; return 1; }
            if (my >= body_y + 46 && my < body_y + 54) { debug_overlay_enabled = !debug_overlay_enabled; return 1; }
        }
    }
    return 0;
}

static int handle_explorer_click(window_t* win, int32_t mx, int32_t my) {
    if (!win) return 0;
    int x = win->x;
    int y = win->y;
    int w = win->w;
    int h = win->h;

    // Up button in header when inside a drive view.
    if (!explorer_this_pc_view &&
        mx >= x + w - 48 && mx < x + w - 8 &&
        my >= y + 25 && my < y + 35) {
        explorer_go_parent();
        return 1;
    }

    // Quick access: This PC
    if (mx >= x + 8 && mx < x + 90 && my >= y + 90 && my < y + 102) {
        if (!storage_initialized) {
            ensure_storage_initialized();
            explorer_build_this_pc_entries();
        }
        explorer_open_this_pc();
        return 1;
    }

    // Main list area
    int list_x = x + 96;
    int list_y = y + 40;
    int list_w = w - 100;
    int list_h = h - 50;
    if (list_w <= 0 || list_h <= 0) return 0;
    if (mx < list_x || mx >= list_x + list_w || my < list_y || my >= list_y + list_h) return 0;

    int row = (my - (y + 44)) / 12;
    if (row < 0) return 1;

    if (explorer_this_pc_view) {
        if (!storage_initialized) {
            ensure_storage_initialized();
            explorer_build_this_pc_entries();
        }
        if (row >= explorer_drive_count) return 1;
        explorer_selected = row;
        strncpy(explorer_path, explorer_drive_paths[row], sizeof(explorer_path) - 1);
        explorer_path[sizeof(explorer_path) - 1] = 0;
        explorer_this_pc_view = 0;
        explorer_refresh();
        return 1;
    }

    if (row >= explorer_entry_count) return 1;
    explorer_selected = row;
    if (explorer_is_dir[row]) {
        strncpy(explorer_path, explorer_entries[row], sizeof(explorer_path) - 1);
        explorer_path[sizeof(explorer_path) - 1] = 0;
        explorer_refresh();
    } else if (path_has_extension(explorer_entries[row], ".EXE")) {
        launch_application_exe(explorer_entries[row], -1, -1);
    } else if (path_has_extension(explorer_entries[row], ".gosapp")) {
        raise_runtime_error("Manifest Info", path_basename(explorer_entries[row]));
    }
    return 1;
}

static int handle_notepad_wheel(window_t* win, int8_t wheel_delta) {
    if (!win || wheel_delta == 0) return 0;
    int text_top = win->y + 36;
    int text_bottom = win->y + win->h - 18;
    int visible_lines = (text_bottom - text_top) / 5;
    if (visible_lines < 1) visible_lines = 1;
    int top = notepad_view_top - (int)wheel_delta;
    if (top < 0) top = 0;
    if (top > NOTEPAD_MAX_LINES - visible_lines) top = NOTEPAD_MAX_LINES - visible_lines;
    if (top < 0) top = 0;
    if (top != notepad_view_top) {
        notepad_view_top = top;
        return 1;
    }
    return 0;
}

static int handle_settings_wheel(window_t* win, int32_t mx, int32_t my, int8_t wheel_delta) {
    if (!win || wheel_delta == 0) return 0;
    int nav_x = win->x + 6;
    int nav_w = 124;
    int content_x = nav_x + nav_w + 8;
    int content_y = win->y + 30;
    int content_w = win->w - (content_x - win->x) - 8;
    int content_h = win->h - 36;
    if (mx < content_x || mx >= content_x + content_w || my < content_y || my >= content_y + content_h) {
        return 0;
    }

    if (settings_tab == SETTINGS_TAB_GAMEROS_UPDATE) {
        settings_md_ensure_loaded();
        int total = settings_md_line_count;
        int start_idx = 0;
        int end_idx = total;
        if (settings_md_plan_start >= 0) {
            if (settings_update_view == SETTINGS_UPDATE_VIEW_CHANGELOG) {
                end_idx = settings_md_plan_start;
            } else if (settings_update_view == SETTINGS_UPDATE_VIEW_ROADMAP) {
                start_idx = settings_md_plan_start;
            }
        }
        if (end_idx < start_idx) end_idx = start_idx;
        int view_count = end_idx - start_idx;
        int visible = (content_h - 50) / 10;
        if (visible < 1) visible = 1;
        int top = settings_scroll_top[SETTINGS_TAB_GAMEROS_UPDATE] - (int)wheel_delta;
        if (top < 0) top = 0;
        if (top > view_count - visible) top = view_count - visible;
        if (top < 0) top = 0;
        if (top != settings_scroll_top[SETTINGS_TAB_GAMEROS_UPDATE]) {
            settings_scroll_top[SETTINGS_TAB_GAMEROS_UPDATE] = top;
            return 1;
        }
    }
    return 0;
}

// Process mouse input
void process_mouse(int32_t mx, int32_t my, uint8_t buttons, int8_t wheel_delta) {
#define RETURN_MOUSE() do { last_buttons = buttons; return; } while (0)
    if (mx < 0) mx = 0;
    if (my < 0) my = 0;
    if (mx >= (int32_t)current_width) mx = (int32_t)current_width - 1;
    if (my >= (int32_t)current_height) my = (int32_t)current_height - 1;
    if (mx < 0 || my < 0) RETURN_MOUSE();

    buttons &= (uint8_t)(MOUSE_BTN_LEFT | MOUSE_BTN_RIGHT);
    if ((buttons & (MOUSE_BTN_LEFT | MOUSE_BTN_RIGHT)) == (MOUSE_BTN_LEFT | MOUSE_BTN_RIGHT)) {
        // Ignore ambiguous dual-button packets quietly to avoid unstable VM popup paths.
        if (!warned_mouse_packet) {
            serial_write_string("Input warning: ambiguous mouse packet detected (L+R together)\n");
            warned_mouse_packet = 1;
        }
        last_buttons = 0;
        return;
    }

    uint8_t pressed = buttons & ~last_buttons;

    if (pressed & MOUSE_BTN_RIGHT) {
        int clicked_idx = find_top_window_at(mx, my);
        int desktop_h = (int)current_height - TASKBAR_HEIGHT;
        if (clicked_idx < 0 && mx >= 0 && mx < (int)current_width && my >= 0 && my < desktop_h) {
            desktop_menu_open = 1;
            desktop_menu_x = mx;
            desktop_menu_y = my;
            start_menu_open = 0;
            RETURN_MOUSE();
        }
        desktop_menu_open = 0;
    }
    
    if ((pressed & MOUSE_BTN_LEFT) && !(buttons & MOUSE_BTN_RIGHT)) {
        if (error_popup_open) {
            int ew = 264, eh = 94;
            int ex = ((int)current_width - ew) / 2;
            int ey = (((int)current_height - TASKBAR_HEIGHT) - eh) / 2;
            if (ex < 4) ex = 4;
            if (ey < 4) ey = 4;
            int ok_x = ex + ew - 58;
            int ok_y = ey + eh - 22;
            if (mx >= ok_x && mx < ok_x + 46 && my >= ok_y && my < ok_y + 14) {
                error_popup_open = 0;
                RETURN_MOUSE();
            }
            RETURN_MOUSE();
        }

        if (debug_overlay_enabled && debug_log_count > 0) {
            int dx = 0, dy = 0, dw = 0, dh = 0;
            get_debug_overlay_rect(&dx, &dy, &dw, &dh);
            if (mx >= dx && mx < dx + dw && my >= dy && my < dy + dh) {
                if (mx >= dx + dw - WINDOW_RESIZE_GRIP && mx < dx + dw &&
                    my >= dy + dh - WINDOW_RESIZE_GRIP && my < dy + dh) {
                    debug_overlay_resizing = 1;
                    debug_overlay_resize_start_w = dw;
                    debug_overlay_resize_start_h = dh;
                    debug_overlay_resize_start_mx = mx;
                    debug_overlay_resize_start_my = my;
                    RETURN_MOUSE();
                }

                if (my >= dy && my < dy + 12) {
                    debug_overlay_dragging = 1;
                    debug_overlay_drag_x = mx - dx;
                    debug_overlay_drag_y = my - dy;
                    RETURN_MOUSE();
                }
            }
        }

        if (desktop_menu_open) {
            desktop_menu_metrics_t dm = {0};
            get_desktop_menu_metrics(&dm);
            if (mx >= dm.x && mx < dm.x + dm.w && my >= dm.y && my < dm.y + dm.h) {
                int rel_y = my - (dm.y + 4);
                int idx = rel_y / dm.item_h;
                if (idx == 0) {
                    launch_application_exe("ABOUT.EXE", -1, -1);
                } else if (idx == 1) {
                    launch_application_exe("SETTINGS.EXE", -1, -1);
                }
            }
            desktop_menu_open = 0;
            RETURN_MOUSE();
        }

        // Start button
        int start_y = (int)current_height - TASKBAR_HEIGHT + ((TASKBAR_HEIGHT - START_BTN_H) / 2);
        if (mx >= START_BTN_X && mx < START_BTN_X + START_BTN_W &&
            my >= start_y && my < start_y + START_BTN_H) {
            start_menu_open = !start_menu_open;
            desktop_menu_open = 0;
            RETURN_MOUSE();
        }

        // Start menu items
        if (start_menu_open) {
            start_menu_metrics_t sm = {0};
            get_start_menu_metrics(&sm);
            int menu_x = sm.x;
            int menu_w = sm.w;
            int menu_h = sm.h;
            int menu_y = sm.y;
            int menu_item_h = sm.item_h;
            int rail_w = 42;
            int panel_x = menu_x + rail_w + 8;
            int panel_w = menu_w - rail_w - 14;
            int items_top = menu_y + 46;
            if (mx >= menu_x && mx < menu_x + menu_w && my >= menu_y && my < menu_y + menu_h) {
                if (mx >= panel_x && mx < panel_x + panel_w &&
                    my >= items_top && my < items_top + menu_item_h * 5) {
                    int idx = (my - items_top) / menu_item_h;
                    if (idx == 0) {
                        launch_application_exe("NOTEPAD.EXE", -1, -1);
                    } else if (idx == 1) {
                        launch_application_exe("SETTINGS.EXE", -1, -1);
                    } else if (idx == 2) {
                        launch_application_exe("EXPLORER.EXE", -1, -1);
                    } else if (idx == 3) {
                        launch_settings_tab(SETTINGS_TAB_GAMEROS_UPDATE);
                    } else if (idx == 4) {
                        launch_application_exe("ABOUT.EXE", -1, -1);
                    }
                } else if (my >= menu_y + menu_h - 24 && my < menu_y + menu_h - 8 &&
                           mx >= panel_x && mx < panel_x + panel_w) {
                    shutdown_os();
                }
                start_menu_open = 0;
                RETURN_MOUSE();
            }
            start_menu_open = 0;
        }

        int clicked_idx = find_top_window_at(mx, my);
        if (clicked_idx >= 0) {
            if (error_popup_open) RETURN_MOUSE();
            desktop_menu_open = 0;
            int tx = windows[clicked_idx].x;
            int ty = windows[clicked_idx].y;
            int tw = windows[clicked_idx].w;
            int th = windows[clicked_idx].h;

            if (mx >= tx + tw - 26 && mx < tx + tw - 6 && my >= ty + 4 && my < ty + 20) {
                close_window(clicked_idx);
                RETURN_MOUSE();
            }

            if (mx >= tx + tw - WINDOW_RESIZE_GRIP && mx < tx + tw &&
                my >= ty + th - WINDOW_RESIZE_GRIP && my < ty + th) {
                windows[clicked_idx].resizing = 1;
                windows[clicked_idx].resize_start_w = windows[clicked_idx].w;
                windows[clicked_idx].resize_start_h = windows[clicked_idx].h;
                windows[clicked_idx].resize_start_mx = mx;
                windows[clicked_idx].resize_start_my = my;
                bring_window_to_front(clicked_idx);
                RETURN_MOUSE();
            }

            if (mx >= tx && mx < tx + tw - 26 && my >= ty && my < ty + 24) {
                windows[clicked_idx].dragging = 1;
                windows[clicked_idx].drag_x = mx - windows[clicked_idx].x;
                windows[clicked_idx].drag_y = my - windows[clicked_idx].y;
                bring_window_to_front(clicked_idx);
                RETURN_MOUSE();
            }

            bring_window_to_front(clicked_idx);
            window_t* focused = &windows[active_window];
            if (focused->type == WIN_SETTINGS && handle_settings_click(focused, mx, my)) {
                RETURN_MOUSE();
            }
            if (focused->type == WIN_EXPLORER && handle_explorer_click(focused, mx, my)) {
                RETURN_MOUSE();
            }
            RETURN_MOUSE();
        }
        
        // Check desktop icons
        for (int i = 0; i < (int)NUM_ICONS; i++) {
            int ix = desktop_icons[i].x;
            int iy = desktop_icons[i].y;
            if (mx >= ix && mx < ix + DESKTOP_ICON_HIT_W &&
                my >= iy && my < iy + DESKTOP_ICON_HIT_H) {
                launch_application_exe(desktop_icons[i].exe_name, 60 + i * 30, 40 + i * 20);
                RETURN_MOUSE();
            }
        }
        
        // Check taskbar
        int taskbar_y = (int)current_height - TASKBAR_HEIGHT;
        int taskbtn_y = taskbar_y + ((TASKBAR_HEIGHT - TASKBTN_H) / 2);
        int taskbtn_w = setting_compact_mode ? TASKBTN_W : (TASKBTN_W + 16);
        int task_x = 56;
        int clock_w = taskbar_clock_width();
        int clock_x = (int)current_width - clock_w - 4;
        int task_right_limit = clock_x - 4;
        for (int i = 0; i < MAX_WINDOWS; i++) {
            if (windows[i].active) {
                if (task_x + taskbtn_w > task_right_limit) {
                    break;
                }
                if (mx >= task_x && mx < task_x + taskbtn_w &&
                    my >= taskbtn_y &&
                    my < taskbtn_y + TASKBTN_H) {
                    bring_window_to_front(i);
                    RETURN_MOUSE();
                }
                task_x += taskbtn_w + 2;
            }
        }
    }
    
    // Handle dragging
    if (buttons & MOUSE_BTN_LEFT) {
        if (debug_overlay_enabled && debug_overlay_dragging) {
            debug_overlay_x = mx - debug_overlay_drag_x;
            debug_overlay_y = my - debug_overlay_drag_y;
        } else if (debug_overlay_enabled && debug_overlay_resizing) {
            int min_w = 160;
            int min_h = 42;
            int max_w = (int)current_width - debug_overlay_x;
            int max_h = ((int)current_height - TASKBAR_HEIGHT) - debug_overlay_y;
            int new_w = debug_overlay_resize_start_w + (mx - debug_overlay_resize_start_mx);
            int new_h = debug_overlay_resize_start_h + (my - debug_overlay_resize_start_my);
            if (new_w < min_w) new_w = min_w;
            if (new_h < min_h) new_h = min_h;
            if (new_w > max_w) new_w = max_w;
            if (new_h > max_h) new_h = max_h;
            debug_overlay_w = new_w;
            debug_overlay_h = new_h;
        }
        for (int i = 0; i < MAX_WINDOWS; i++) {
            if (windows[i].active && windows[i].dragging) {
                windows[i].x = mx - windows[i].drag_x;
                windows[i].y = my - windows[i].drag_y;
                if (windows[i].x < 0) windows[i].x = 0;
                if (windows[i].y < 0) windows[i].y = 0;
                if (windows[i].x > (int)current_width - windows[i].w) windows[i].x = current_width - windows[i].w;
                if (windows[i].y > (int)current_height - TASKBAR_HEIGHT - 20) {
                    windows[i].y = (int)current_height - TASKBAR_HEIGHT - 20;
                }
            } else if (windows[i].active && windows[i].resizing) {
                int min_w = WINDOW_MIN_W_DEFAULT;
                int min_h = WINDOW_MIN_H_DEFAULT;
                get_window_min_size(&windows[i], &min_w, &min_h);
                int max_w = (int)current_width - windows[i].x;
                int max_h = ((int)current_height - TASKBAR_HEIGHT) - windows[i].y;
                int new_w = windows[i].resize_start_w + (mx - windows[i].resize_start_mx);
                int new_h = windows[i].resize_start_h + (my - windows[i].resize_start_my);
                if (new_w < min_w) new_w = min_w;
                if (new_h < min_h) new_h = min_h;
                if (new_w > max_w) new_w = max_w;
                if (new_h > max_h) new_h = max_h;
                windows[i].w = new_w;
                windows[i].h = new_h;
            }
        }
    }
    
    // Stop dragging
    if ((~buttons & last_buttons) & MOUSE_BTN_LEFT) {
        debug_overlay_dragging = 0;
        debug_overlay_resizing = 0;
        for (int i = 0; i < MAX_WINDOWS; i++) {
            windows[i].dragging = 0;
            windows[i].resizing = 0;
        }
    }

    if (wheel_delta != 0) {
        int hovered_idx = find_top_window_at(mx, my);
        if (hovered_idx >= 0 && windows[hovered_idx].active) {
            window_t* hovered = &windows[hovered_idx];
            if (hovered->type == WIN_NOTEPAD) {
                (void)handle_notepad_wheel(hovered, wheel_delta);
            } else if (hovered->type == WIN_SETTINGS) {
                (void)handle_settings_wheel(hovered, mx, my, wheel_delta);
            }
        }
    }
    
    last_buttons = buttons;
#undef RETURN_MOUSE
}

// Process keyboard input
int process_keyboard(void) {
    if (active_window < 0 || !windows[active_window].active) return 0;
    
    window_t* win = &windows[active_window];
    int changed = 0;
    
    while (1) {
        char c = keyboard_getchar();
        if (!c) break;
        
        if (win->type == WIN_NOTEPAD) {
            if ((uint8_t)c == KEY_PGUP) {
                if (!storage_initialized) ensure_storage_initialized();
                if (storage_initialized) notepad_save_to_storage();
                changed = 1;
            } else if ((uint8_t)c == KEY_PGDN) {
                if (!storage_initialized) ensure_storage_initialized();
                if (storage_initialized) notepad_load_from_storage();
                changed = 1;
            } else if ((uint8_t)c == KEY_DEL) {
                if (notepad_cursor_x < NOTEPAD_MAX_COLS - 1) {
                    for (int x = notepad_cursor_x; x < NOTEPAD_MAX_COLS - 1; x++) {
                        notepad_lines[notepad_cursor_y][x] = notepad_lines[notepad_cursor_y][x + 1];
                    }
                    notepad_lines[notepad_cursor_y][NOTEPAD_MAX_COLS - 1] = 0;
                    notepad_dirty = 1;
                    changed = 1;
                }
            } else if (c == '\b') {
                if (notepad_cursor_x > 0) {
                    notepad_cursor_x--;
                    notepad_lines[notepad_cursor_y][notepad_cursor_x] = 0;
                    notepad_dirty = 1;
                    changed = 1;
                } else if (notepad_cursor_y > 0) {
                    notepad_cursor_y--;
                    notepad_cursor_x = NOTEPAD_MAX_COLS - 1;
                    while (notepad_cursor_x > 0 && !notepad_lines[notepad_cursor_y][notepad_cursor_x - 1]) {
                        notepad_cursor_x--;
                    }
                    notepad_dirty = 1;
                    changed = 1;
                }
            } else if ((uint8_t)c == KEY_LEFT) {
                if (notepad_cursor_x > 0) {
                    notepad_cursor_x--;
                } else if (notepad_cursor_y > 0) {
                    notepad_cursor_y--;
                    notepad_cursor_x = NOTEPAD_MAX_COLS - 1;
                    while (notepad_cursor_x > 0 &&
                           notepad_lines[notepad_cursor_y][notepad_cursor_x - 1] == 0) {
                        notepad_cursor_x--;
                    }
                }
                changed = 1;
            } else if ((uint8_t)c == KEY_RIGHT) {
                if (notepad_cursor_x < NOTEPAD_MAX_COLS - 1) {
                    notepad_cursor_x++;
                } else if (notepad_cursor_y < NOTEPAD_MAX_LINES - 1) {
                    notepad_cursor_y++;
                    notepad_cursor_x = 0;
                }
                changed = 1;
            } else if ((uint8_t)c == KEY_UP) {
                if (notepad_cursor_y > 0) {
                    notepad_cursor_y--;
                    if (notepad_cursor_x >= NOTEPAD_MAX_COLS) {
                        notepad_cursor_x = NOTEPAD_MAX_COLS - 1;
                    }
                }
                changed = 1;
            } else if ((uint8_t)c == KEY_DOWN) {
                if (notepad_cursor_y < NOTEPAD_MAX_LINES - 1) {
                    notepad_cursor_y++;
                    if (notepad_cursor_x >= NOTEPAD_MAX_COLS) {
                        notepad_cursor_x = NOTEPAD_MAX_COLS - 1;
                    }
                }
                changed = 1;
            } else if ((uint8_t)c == KEY_HOME) {
                notepad_cursor_x = 0;
                changed = 1;
            } else if ((uint8_t)c == KEY_END) {
                int end = 0;
                while (end < NOTEPAD_MAX_COLS && notepad_lines[notepad_cursor_y][end]) {
                    end++;
                }
                notepad_cursor_x = end;
                changed = 1;
            } else if (c == '\n') {
                notepad_cursor_y++;
                notepad_cursor_x = 0;
                if (notepad_cursor_y >= NOTEPAD_MAX_LINES) notepad_cursor_y = NOTEPAD_MAX_LINES - 1;
                notepad_dirty = 1;
                changed = 1;
            } else if (c >= 32 && c < 127) {
                notepad_lines[notepad_cursor_y][notepad_cursor_x] = c;
                notepad_cursor_x++;
                if (notepad_cursor_x >= NOTEPAD_MAX_COLS) {
                    notepad_cursor_x = 0;
                    notepad_cursor_y++;
                    if (notepad_cursor_y >= NOTEPAD_MAX_LINES) notepad_cursor_y = NOTEPAD_MAX_LINES - 1;
                }
                notepad_dirty = 1;
                changed = 1;
            }

            // Keep edit caret in visible viewport after keyboard movement/editing.
            if (notepad_cursor_y < notepad_view_top) {
                notepad_view_top = notepad_cursor_y;
            } else {
                int text_top = win->y + 36;
                int text_bottom = win->y + win->h - 18;
                int visible_lines = (text_bottom - text_top) / 5;
                if (visible_lines < 1) visible_lines = 1;
                if (notepad_cursor_y >= notepad_view_top + visible_lines) {
                    notepad_view_top = notepad_cursor_y - visible_lines + 1;
                }
            }
        }
    }
    return changed;
}

static void cursor_restore_background(void) {
    if (!cursor_drawn_valid) return;
    for (int row = 0; row < CURSOR_SIZE; row++) {
        for (int col = 0; col < CURSOR_SIZE; col++) {
            int idx = row * CURSOR_SIZE + col;
            if (graphics_is_truecolor()) {
                draw_pixel_rgb(cursor_drawn_x + col, cursor_drawn_y + row, cursor_under_rgb[idx]);
            } else {
                draw_pixel(cursor_drawn_x + col, cursor_drawn_y + row, (uint8_t)(cursor_under_rgb[idx] & 0xFF));
            }
        }
    }
}

static void cursor_capture_background(int32_t x, int32_t y) {
    for (int row = 0; row < CURSOR_SIZE; row++) {
        for (int col = 0; col < CURSOR_SIZE; col++) {
            int idx = row * CURSOR_SIZE + col;
            if (graphics_is_truecolor()) {
                cursor_under_rgb[idx] = graphics_get_pixel_rgb(x + col, y + row);
            } else {
                cursor_under_rgb[idx] = vga_get_pixel(x + col, y + row);
            }
        }
    }
}

static void cursor_compose_on_backbuffer(int32_t x, int32_t y) {
    cursor_restore_background();
    cursor_capture_background(x, y);
    draw_cursor(x, y);
    cursor_drawn_x = x;
    cursor_drawn_y = y;
    cursor_drawn_valid = 1;
}

// Main kernel entry
void kernel_main(multiboot_info_t* mb_info) {
    serial_init();
    serial_write_string("GamerOS Starting...\n");
    debug_log_message("Boot: kernel_main start");
    
    // Prefer safe-validated multiboot framebuffer path; fallback to stable VGA renderer.
    if (!graphics_use_multiboot_framebuffer(mb_info)) {
        set_video_mode(MODE_VESA_640x480);
        debug_log_message("Boot: using VGA fallback");
    } else {
        debug_log_message("Boot: using framebuffer");
    }
    {
        char gfx_boot_line[72];
        sprintf(gfx_boot_line, "Graphics mode: %ux%ux%u %s\n",
                (unsigned int)current_width,
                (unsigned int)current_height,
                (unsigned int)graphics_get_bpp(),
                graphics_is_truecolor() ? "RGBA" : "Indexed");
        serial_write_string(gfx_boot_line);
        debug_log_message(gfx_boot_line);
    }
    
    // Initialize subsystems
    keyboard_init();
    debug_log_message("Boot: keyboard init ok");
    mouse_init();
    debug_log_message("Boot: mouse init ok");
    // Seed desktop state before storage comes online.
    explorer_drive_count = 1;
    explorer_drive_paths[0][0] = 'C';
    explorer_drive_paths[0][1] = ':';
    explorer_drive_paths[0][2] = '/';
    explorer_drive_paths[0][3] = 0;
    strncpy(explorer_drive_labels[0], "C: Local Disk", sizeof(explorer_drive_labels[0]) - 1);
    explorer_drive_labels[0][sizeof(explorer_drive_labels[0]) - 1] = 0;
    init_windows();
    debug_log_message("Boot: windows init ok");
    startup_animation();
    debug_log_message("Boot: startup animation done");
    debug_present_boot_status();
    debug_log_message("Boot: storage deferred");
    debug_log_message("Boot: entering main loop");
    debug_present_boot_status();

    // VMware stability mode: use polling-only input in the main loop.
    // IRQ-driven mouse/keyboard paths can still trigger fault storms while dragging.
    __asm__ volatile ("cli");
    
    // Main loop
    int32_t last_mx = -1, last_my = -1;
    uint8_t scene_dirty = 0;

    // Paint one desktop frame before the first input poll so a noisy PS/2
    // controller cannot trap the VM on a black handoff screen.
    {
        int32_t boot_mx = mouse_get_x();
        int32_t boot_my = mouse_get_y();
        if (boot_mx < 0) boot_mx = 0;
        if (boot_my < 0) boot_my = 0;
        if (boot_mx >= (int32_t)current_width) boot_mx = (int32_t)current_width - 1;
        if (boot_my >= (int32_t)current_height) boot_my = (int32_t)current_height - 1;
        clear_screen(0);
        draw_desktop();
        cursor_compose_on_backbuffer(boot_mx, boot_my);
        swap_buffers();
        last_mx = boot_mx;
        last_my = boot_my;
        debug_log_message("Boot: desktop frame ready");
    }
    
    while (1) {
        if (shutdown_requested) {
            // Stay on shutdown screen without triggering VMware "CPU disabled" popup.
            for (;;) {
                __asm__ volatile ("pause");
            }
        }

        mouse_poll();
        keyboard_poll();
        
        int32_t raw_mx = mouse_get_x();
        int32_t raw_my = mouse_get_y();
        int32_t mx = raw_mx;
        int32_t my = raw_my;
        if (!warned_mouse_bounds &&
            (raw_mx < 0 || raw_my < 0 ||
             raw_mx >= (int32_t)current_width ||
             raw_my >= (int32_t)current_height)) {
            raise_runtime_error("Input Bounds Error", "Mouse coordinates were outside screen bounds.");
            warned_mouse_bounds = 1;
        }
        if (mx < 0) mx = 0;
        if (my < 0) my = 0;
        if (mx >= (int32_t)current_width) mx = (int32_t)current_width - 1;
        if (my >= (int32_t)current_height) my = (int32_t)current_height - 1;
        uint8_t buttons = mouse_get_buttons();
        // Normalize wheel direction so wheel-up scrolls content up in UI lists/editors.
        int8_t wheel_delta = (int8_t)(-mouse_get_wheel_delta());
        uint8_t prev_buttons = last_buttons;
        
        process_mouse(mx, my, buttons, wheel_delta);
        int keyboard_changed = process_keyboard();

        int mouse_moved = (mx != last_mx || my != last_my);
        int buttons_changed = (buttons != prev_buttons);
        int dragging_frame = ((buttons & MOUSE_BTN_LEFT) && mouse_moved) ? 1 : 0;
        int full_redraw = (scene_dirty || keyboard_changed || wheel_delta != 0 || buttons_changed || dragging_frame) ? 1 : 0;

        if (full_redraw) {
            cursor_restore_background();
            cursor_drawn_valid = 0;
            clear_screen(0);
            draw_desktop();
            cursor_compose_on_backbuffer(mx, my);
            swap_buffers();
            scene_dirty = 0;
        } else if (mouse_moved) {
            int32_t prev_x = cursor_drawn_x;
            int32_t prev_y = cursor_drawn_y;
            uint8_t had_prev = cursor_drawn_valid;

            cursor_compose_on_backbuffer(mx, my);

            if (had_prev) {
                int left = (prev_x < mx) ? prev_x : mx;
                int top = (prev_y < my) ? prev_y : my;
                int right = ((prev_x > mx) ? prev_x : mx) + CURSOR_SIZE;
                int bottom = ((prev_y > my) ? prev_y : my) + CURSOR_SIZE;
                present_rect(left, top, right - left, bottom - top);
            } else {
                present_rect(mx, my, CURSOR_SIZE, CURSOR_SIZE);
            }
        }

        last_mx = mx;
        last_my = my;
        last_buttons = buttons;

        // Tiny idle pause to reduce VM jitter when frame is unchanged.
        if (!full_redraw && !mouse_moved && !keyboard_changed && wheel_delta == 0) {
            for (volatile int i = 0; i < 1200; i++) {
                __asm__ volatile ("pause");
            }
        }
    }
}

// TODO: Replace polling-only desktop loop with timer-driven frame pacing to reduce idle CPU usage.
// TODO: Derive scene_dirty from centralized event queue instead of scattered state checks.
