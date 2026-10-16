#include <graphics.h>
#include <mouse.h>
#include <keyboard.h>
#include <serial.h>
#include <ports.h>
#include <rtc.h>
#include <timezone.h>
#include <font.h>
#include <fs.h>
#include <ata_pio.h>
#include <apps.h>
#include <shell_api.h>
#include <markdown.h>
#include <notepad_app.h>
#include <process_model.h>
#include <multiboot.h>
#include <multiboot2.h>
#include <settings_ui.h>
#include <usb.h>
#include <xhci.h>
#include <vmware.h>
#include <vboxguest.h>
#include "../../resources/wallpapers/background_wallpaper.h"
#include <stdint.h>
#include <string.h>

// Window kinds, mirroring app_descriptor_t window_type values.
#define APP_WINDOW_NOTEPAD 1
#define APP_WINDOW_MYCOMP 2
#define APP_WINDOW_ABOUT 3
#define APP_WINDOW_SETTINGS 4
#define APP_WINDOW_EXPLORER 5

#define MAX_WINDOWS 6
#define WIN_NONE    0
#define WIN_NOTEPAD APP_WINDOW_NOTEPAD
#define WIN_MYCOMP  APP_WINDOW_MYCOMP
#define WIN_ABOUT   APP_WINDOW_ABOUT
#define WIN_SETTINGS APP_WINDOW_SETTINGS
#define WIN_EXPLORER APP_WINDOW_EXPLORER

#define START_BTN_X            4
#define START_BTN_W            64
#define START_BTN_H            16
#define TASKBTN_W              70
#define TASKBTN_H              16
#define START_MENU_RAIL_W      56
#define START_MENU_W           248
#define START_MENU_H           176
#define START_MENU_ITEM_H      20
#define START_MENU_ITEMS_TOP   50
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
static int setting_display_profile = 0;

#define EXPLORER_MAX_ENTRIES 14
#define EXPLORER_DRIVE_ROW_H 34
#define EXPLORER_BAR_H       7
static char explorer_path[MAX_FILENAME_LEN] = "C:/";
static char explorer_entries[EXPLORER_MAX_ENTRIES][MAX_FILENAME_LEN];
static uint8_t explorer_is_dir[EXPLORER_MAX_ENTRIES];
static int explorer_entry_count = 0;
static int explorer_selected = -1;
static uint8_t explorer_this_pc_view = 1;
static char explorer_drive_paths[EXPLORER_MAX_ENTRIES][4];
static char explorer_drive_labels[EXPLORER_MAX_ENTRIES][32];
static int explorer_drive_count = 0;

#define EXPLORER_HISTORY 16
static char explorer_hist_path[EXPLORER_HISTORY][MAX_FILENAME_LEN];
static uint8_t explorer_hist_thispc[EXPLORER_HISTORY];
static int explorer_hist_count = 0;
static int explorer_hist_pos = -1;

// Hit boxes published by draw_window() for Explorer chrome.
#define EXPLORER_TB_BUTTONS 4
enum { EXPLORER_TB_BACK = 0, EXPLORER_TB_FWD, EXPLORER_TB_UP, EXPLORER_TB_REFRESH };
static int explorer_tb_x[EXPLORER_TB_BUTTONS];
static int explorer_tb_enabled[EXPLORER_TB_BUTTONS];
static int explorer_tb_y = 0, explorer_tb_w = 0, explorer_tb_h = 0;

#define EXPLORER_SIDEBAR_ROWS 4
static int explorer_sb_x = 0, explorer_sb_w = 0;
static int explorer_sb_hit_y[EXPLORER_SIDEBAR_ROWS];
static int explorer_sb_hit_h = 0;
static int explorer_sb_selected = -1;

static int explorer_list_x = 0, explorer_list_y = 0, explorer_list_w = 0;
static int explorer_list_hit_y[EXPLORER_MAX_ENTRIES];
static int explorer_list_hit_count = 0;
static int explorer_list_hit_h = 0;

static uint8_t storage_initialized = 0;
static uint32_t cursor_under_rgb[CURSOR_SIZE * CURSOR_SIZE];
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
static int debug_overlay_w = 8 + (DEBUG_LOG_CHARS * FONT_CW);
static int debug_overlay_h = 112;
static uint8_t debug_overlay_dragging = 0;
static uint8_t debug_overlay_resizing = 0;
static int debug_overlay_drag_x = 0;
static int debug_overlay_drag_y = 0;
static int debug_overlay_resize_start_w = 0;
static int debug_overlay_resize_start_h = 0;
static int debug_overlay_resize_start_mx = 0;
static int debug_overlay_resize_start_my = 0;

// App lifecycle state for first-class apps.
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

    // Find the descriptor owning this window type.
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

// Maps GOS:/User, GOS:/System and GOS:/Apps prefixes onto real paths.
static int gos_path_resolve(const char* gos_path, char* out_fs_path, size_t out_cap) {
    if (!gos_path || !out_fs_path || out_cap == 0) return 0;

    // Already a concrete path: copy it through.
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
        // Unknown sub-prefix: fall back to the C: root.
        base_fs = "C:/";
    }

    size_t base_len = strlen(base_fs);
    size_t rest_len = strlen(rest);
    if (base_len + rest_len + 1 > out_cap) {
        if (out_cap <= base_len + 1) {
            // Not even room for the base path plus terminator.
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
    int time_chars = settings_app_show_seconds() ? 8 : 5; // HH:MM:SS or HH:MM
    int date_chars = 10; // DD/MM/YYYY
    return ((time_chars + 1 + date_chars) * FONT_CW) + 8;
}

// ---- Dock geometry ---------------------------------------------------------
#define DOCK_PAD_X       8
#define DOCK_PAD_Y       5
#define DOCK_GAP_BOTTOM  7
#define DOCK_GAP_ITEMS   6
#define DOCK_RADIUS      8
#define DOCK_SHADOW      3

// Vertical space the dock claims from the bottom edge.
#define DOCK_INNER_H     ((TASKBTN_H > START_BTN_H) ? TASKBTN_H : START_BTN_H)
#define DOCK_RESERVED_H  (DOCK_INNER_H + (DOCK_PAD_Y * 2) + DOCK_GAP_BOTTOM)

typedef struct {
    int x, y, w, h;
    int start_x, start_y;
    int taskbtn_w, taskbtn_y;
    int task_x, task_count;
    int clock_x, clock_y, clock_w;
} dock_layout_t;

// Taskbar shape and density come from the Settings app.
static void dock_compute_layout(dock_layout_t* d) {
    if (!d) return;

    int taskbtn_w = settings_app_taskbar_compact() ? TASKBTN_W : (TASKBTN_W + 16);
    int clock_w = taskbar_clock_width();

    int inner_h = (TASKBTN_H > START_BTN_H) ? TASKBTN_H : START_BTN_H;
    int h = inner_h + (DOCK_PAD_Y * 2);

    // Start and the clock are fixed width, so they claim space first.
    int fixed_w = (DOCK_PAD_X * 2) + START_BTN_W + clock_w;
    int max_w = (int)current_width - fixed_w - (DOCK_GAP_ITEMS * 2) - 8;
    if (max_w < 0) max_w = 0;
    int max_count = max_w / (taskbtn_w + 2);

    int task_count = 0;
    for (int i = 0; i < MAX_WINDOWS && task_count < max_count; i++) {
        if (windows[i].active) task_count++;
    }

    if (settings_app_taskbar_style() == TASKBAR_STYLE_WINDOWS) {
        // Classic taskbar: full-width bar flush with the bottom edge.
        d->x = 0;
        d->y = (int)current_height - h;
        if (d->y < 0) d->y = 0;
        d->w = (int)current_width;
        d->h = h;
        d->start_x = DOCK_PAD_X;
        d->start_y = d->y + DOCK_PAD_Y;
        d->taskbtn_w = taskbtn_w;
        d->taskbtn_y = d->y + DOCK_PAD_Y;
        d->task_x = d->start_x + START_BTN_W + DOCK_GAP_ITEMS;
        d->task_count = task_count;
        d->clock_w = clock_w;
        d->clock_x = d->w - DOCK_PAD_X - clock_w;
        d->clock_y = d->y + DOCK_PAD_Y;
        return;
    }

    int run_w = task_count * taskbtn_w;
    if (task_count > 1) run_w += (task_count - 1) * 2;
    int gaps = (task_count > 0) ? (DOCK_GAP_ITEMS * 2) : DOCK_GAP_ITEMS;
    int w = fixed_w + run_w + gaps;
    if (w > (int)current_width - 8) w = (int)current_width - 8;
    if (w < 32) w = 32;

    int x = ((int)current_width - w) / 2;
    if (x < 4) x = 4;
    int y = (int)current_height - h - DOCK_GAP_BOTTOM;
    if (y < 0) y = 0;

    d->x = x;
    d->y = y;
    d->w = w;
    d->h = h;
    d->start_x = x + DOCK_PAD_X;
    d->start_y = y + DOCK_PAD_Y;
    d->taskbtn_w = taskbtn_w;
    d->taskbtn_y = y + DOCK_PAD_Y;
    d->task_x = d->start_x + START_BTN_W + DOCK_GAP_ITEMS;
    d->task_count = task_count;
    d->clock_w = clock_w;
    d->clock_x = x + w - DOCK_PAD_X - clock_w;
    d->clock_y = y + DOCK_PAD_Y;
}

typedef struct {
    int x;
    int y;
    int w;
    int h;
    int item_h;
    // Shared by the renderer and the click handler.
    int rail_w;
    int panel_x;
    int panel_w;
    int items_top;
    int item_count;
} start_menu_metrics_t;

typedef struct {
    int x;
    int y;
    int w;
    int h;
    int item_h;
} desktop_menu_metrics_t;

void append_string(char* dest, size_t dest_cap, const char* src);
static void initialize_storage_layout(void);
static file_t* fs_open_or_create_file(const char* path);
static void write_text_file(const char* path, const char* txt);
static void apply_display_profile(void);
static void debug_log_message(const char* message);
static void draw_debug_overlay(void);
static void debug_present_boot_status(void);
static void get_debug_overlay_rect(int* out_x, int* out_y, int* out_w, int* out_h);
static void get_window_min_size(const window_t* win, int* out_w, int* out_h);
static void explorer_refresh(void);
static void explorer_history_push(uint8_t truncate_forward);
static int explorer_can_go_back(void);
static int explorer_can_go_forward(void);
static void explorer_history_step(int delta);
static void explorer_history_reset(void);
static void explorer_navigate_to(const char* path);
static const char* explorer_quick_path(int row);
static const char* explorer_quick_label(int row);
static void explorer_go_parent(void);
static void draw_desktop_watermark(void);
static void draw_desktop_wallpaper(int desktop_h);
static int get_ui_scale(void);
static void get_start_menu_metrics(start_menu_metrics_t* m);
static void get_desktop_menu_metrics(desktop_menu_metrics_t* m);
static void raise_runtime_error(const char* title, const char* message);
static void launch_application_exe(const char* exe_name, int fallback_x, int fallback_y);
static void launch_settings_tab(int tab_idx);
void fill_chamfer_rect(int x, int y, int w, int h, uint8_t color);
void draw_chamfer_rect(int x, int y, int w, int h, uint8_t color);
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

// truncate_forward drops redo entries after the current position.
static void explorer_history_push(uint8_t truncate_forward) {
    if (explorer_hist_count > 0) {
        // Skip a duplicate of the current entry.
        if (explorer_hist_pos >= 0 && explorer_hist_pos < explorer_hist_count &&
            strcmp(explorer_hist_path[explorer_hist_pos], explorer_path) == 0 &&
            explorer_hist_thispc[explorer_hist_pos] == explorer_this_pc_view) {
            return;
        }
    }
    if (truncate_forward) {
        // Keep history up to and including the current position.
        if (explorer_hist_pos >= 0 && explorer_hist_pos < explorer_hist_count) {
            explorer_hist_count = explorer_hist_pos + 1;
        }
    }
    if (explorer_hist_count >= EXPLORER_HISTORY) {
        for (int i = 1; i < EXPLORER_HISTORY; i++) {
            strncpy(explorer_hist_path[i - 1], explorer_hist_path[i], MAX_FILENAME_LEN);
            explorer_hist_thispc[i - 1] = explorer_hist_thispc[i];
        }
        explorer_hist_count = EXPLORER_HISTORY - 1;
    }
    strncpy(explorer_hist_path[explorer_hist_count], explorer_path, MAX_FILENAME_LEN);
    explorer_hist_path[explorer_hist_count][MAX_FILENAME_LEN - 1] = 0;
    explorer_hist_thispc[explorer_hist_count] = explorer_this_pc_view;
    explorer_hist_count++;
    explorer_hist_pos = explorer_hist_count - 1;
}

static int explorer_can_go_back(void) { return explorer_hist_pos > 0; }
static int explorer_can_go_forward(void) {
    return explorer_hist_pos >= 0 && explorer_hist_pos < explorer_hist_count - 1;
}

static void explorer_history_step(int delta) {
    int target = explorer_hist_pos + delta;
    if (target < 0 || target >= explorer_hist_count) return;
    explorer_hist_pos = target;
    strncpy(explorer_path, explorer_hist_path[target], sizeof(explorer_path) - 1);
    explorer_path[sizeof(explorer_path) - 1] = 0;
    explorer_this_pc_view = explorer_hist_thispc[target];
    explorer_selected = -1;
    explorer_refresh();
}

static void explorer_history_reset(void) {
    explorer_hist_count = 0;
    explorer_hist_pos = -1;
}

static void explorer_open_this_pc(void) {
    explorer_this_pc_view = 1;
    explorer_selected = -1;
    explorer_entry_count = 0;
}

// Tenths of a GiB, shifts only: no libgcc means no 64-bit division.
static uint64_t bytes_to_gb_tenths(uint64_t bytes) {
    return (bytes * 10) >> 30;
}

void append_unsigned(char* out, int out_size, int* pos, uint64_t v) {
    char tmp[24];
    int n = 0;
    if (v == 0) tmp[n++] = '0';
    while (v > 0 && n < (int)sizeof(tmp)) {
        tmp[n++] = (char)('0' + (int)(v % 10));
        v /= 10;
    }
    while (n > 0 && *pos + 1 < out_size) {
        out[(*pos)++] = tmp[--n];
    }
}

void append_text(char* out, int out_size, int* pos, const char* s) {
    if (!s) return;
    while (*s && *pos + 1 < out_size) out[(*pos)++] = *s++;
}

// Volumes under 1 GiB are reported in MB, not "0.0 GB".
static void format_capacity_line(char* out, int out_size, uint64_t free_bytes, uint64_t total_bytes) {
    int pos = 0;
    if (!out || out_size <= 1) return;
    out[0] = 0;

    const uint64_t one_gib = 1ULL << 30;

    if (total_bytes < one_gib) {
        uint64_t free_mb = free_bytes >> 20;
        uint64_t total_mb = total_bytes >> 20;
        append_unsigned(out, out_size, &pos, free_mb);
        append_text(out, out_size, &pos, " MB free of ");
        if (total_mb == 0 && total_bytes > 0) total_mb = 1;
        append_unsigned(out, out_size, &pos, total_mb);
        append_text(out, out_size, &pos, " MB");
        out[pos] = 0;
        return;
    }

    uint64_t free_tenths = bytes_to_gb_tenths(free_bytes);
    append_unsigned(out, out_size, &pos, free_tenths / 10);
    if (pos + 1 < out_size) out[pos++] = '.';
    append_unsigned(out, out_size, &pos, free_tenths % 10);
    append_text(out, out_size, &pos, " GB free of ");

    uint64_t total_tenths = bytes_to_gb_tenths(total_bytes);
    uint64_t total_whole = total_tenths / 10;
    if (total_whole == 0 && total_bytes > 0) total_whole = 1;
    if ((total_tenths % 10) != 0) total_whole++;   // round partial GB up
    append_unsigned(out, out_size, &pos, total_whole);
    append_text(out, out_size, &pos, " GB");
    out[pos] = 0;
}


static void ensure_storage_initialized(void) {
    if (storage_initialized) return;
    debug_log_message("Boot: storage init begin");
    debug_present_boot_status();
    initialize_storage_layout();

    {
        fs_space_info_t space;
        if (fs_get_space_info(&space)) {
            serial_write_string("Storage: ");
            serial_write_string(space.persistent ? "ATA medium" : "RAM fallback");
            serial_write_string(", capacity ");
            char cap[24];
            uint64_t whole_gb = (space.total_bytes + (1ULL << 29)) >> 30;
            int cp = 0;
            if (whole_gb == 0) cap[cp++] = '0';
            while (whole_gb > 0 && cp < (int)sizeof(cap)) {
                cap[cp++] = (char)('0' + (int)(whole_gb % 10));
                whole_gb /= 10;
            }
            while (cp > 0) serial_putchar(cap[--cp]);
            serial_write_string(" GB, LBA48=");
            serial_write_string(ata_pio_has_lba48() ? "yes" : "no");
            if (ata_pio_get_model()[0]) {
                serial_write_string(", model=");
                serial_write_string(ata_pio_get_model());
            }
            serial_write_string("\nStorage: files=");
            char num[16];
            int np = 0;
            uint32_t fc = space.file_count;
            if (fc == 0) num[np++] = '0';
            while (fc > 0 && np < (int)sizeof(num)) {
                num[np++] = (char)('0' + (int)(fc % 10));
                fc /= 10;
            }
            while (np > 0) serial_putchar(num[--np]);
            serial_write_string(" dirs=");
            uint32_t dc = space.dir_count;
            np = 0;
            if (dc == 0) num[np++] = '0';
            while (dc > 0 && np < (int)sizeof(num)) {
                num[np++] = (char)('0' + (int)(dc % 10));
                dc /= 10;
            }
            while (np > 0) serial_putchar(num[--np]);
            serial_write_string(", used=");
            {
                uint64_t used_kb = space.used_bytes >> 10;
                np = 0;
                if (used_kb == 0) num[np++] = '0';
                while (used_kb > 0 && np < (int)sizeof(num)) {
                    num[np++] = (char)('0' + (int)(used_kb % 10));
                    used_kb /= 10;
                }
                while (np > 0) serial_putchar(num[--np]);
            }
            serial_write_string(" KB\n");
        }
    }

    storage_initialized = 1;
    settings_app_reload_changelog();
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
        if (windows[i].h > (int)current_height - DOCK_RESERVED_H - 8) windows[i].h = (int)current_height - DOCK_RESERVED_H - 8;
        if (windows[i].w < WINDOW_MIN_W_DEFAULT) windows[i].w = WINDOW_MIN_W_DEFAULT;
        if (windows[i].h < WINDOW_MIN_H_DEFAULT) windows[i].h = WINDOW_MIN_H_DEFAULT;
        if (windows[i].x > (int)current_width - windows[i].w) windows[i].x = (int)current_width - windows[i].w;
        if (windows[i].y > (int)current_height - DOCK_RESERVED_H - windows[i].h) {
            windows[i].y = (int)current_height - DOCK_RESERVED_H - windows[i].h;
        }
        if (windows[i].x < 0) windows[i].x = 0;
        if (windows[i].y < 0) windows[i].y = 0;
    }
}

const char* path_basename(const char* path) {
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

int path_has_extension(const char* path, const char* ext) {
    if (!path || !ext) return 0;
    size_t path_len = strlen(path);
    size_t ext_len = strlen(ext);
    if (path_len < ext_len) return 0;
    return strcmp(path + (path_len - ext_len), ext) == 0;
}

// Details-view column widths (px); Name absorbs the remainder.
#define EXPL_COL_DATE_W 152
#define EXPL_COL_TYPE_W 124
#define EXPL_COL_SIZE_W 86

static void format_mtime(char* out, int out_size, uint32_t unix_time) {
    int pos = 0;
    if (!out || out_size <= 1) return;
    out[0] = 0;

    uint16_t year = 1970;
    uint8_t month = 1, day = 1, hour = 0, minute = 0;
    unix_to_calendar(unix_time, &year, &month, &day, &hour, &minute);

    append_unsigned(out, out_size, &pos, month);
    if (pos + 1 < out_size) out[pos++] = '/';
    append_unsigned(out, out_size, &pos, day);
    if (pos + 1 < out_size) out[pos++] = '/';
    append_unsigned(out, out_size, &pos, year);
    if (pos + 1 < out_size) out[pos++] = ' ';

    int h12 = (int)hour;
    const char* suffix = "AM";
    if (h12 == 0) h12 = 12;
    else if (h12 >= 12) { suffix = "PM"; if (h12 > 12) h12 -= 12; }
    append_unsigned(out, out_size, &pos, (uint64_t)h12);
    if (pos + 1 < out_size) out[pos++] = ':';
    if (minute < 10 && pos + 1 < out_size) out[pos++] = '0';
    append_unsigned(out, out_size, &pos, minute);
    if (pos + 1 < out_size) out[pos++] = ' ';
    append_text(out, out_size, &pos, suffix);
    out[pos] = 0;
}

static const char* file_type_label(const char* path, int is_dir) {
    if (is_dir) return "File folder";
    const char* base = path_basename(path);
    if (path_has_extension(base, ".EXE")) return "Application";
    if (path_has_extension(base, ".GOSAPP")) return "Application manifest";
    if (path_has_extension(base, ".DLL")) return "Application extension";
    if (path_has_extension(base, ".TXT")) return "Text Document";
    if (path_has_extension(base, ".MD")) return "Markdown Document";
    if (path_has_extension(base, ".INI")) return "Configuration Settings";
    if (path_has_extension(base, ".REG")) return "Registry File";
    if (path_has_extension(base, ".LST")) return "System File";
    if (path_has_extension(base, ".LOG")) return "Log File";
    if (path_has_extension(base, ".SYS")) return "System file";
    return "File";
}

// Shifts only, no 64-bit division.
static void format_file_size(char* out, int out_size, uint32_t bytes) {
    int pos = 0;
    if (!out || out_size <= 1) return;
    out[0] = 0;

    if (bytes < 1024u) {
        append_unsigned(out, out_size, &pos, bytes);
        append_text(out, out_size, &pos, " bytes");
        out[pos] = 0;
        return;
    }

    uint64_t value;
    const char* unit;
    if (bytes < (1024u * 1024u)) {
        value = ((uint64_t)bytes * 10) >> 10;   // tenths of a KB
        unit = " KB";
    } else {
        value = ((uint64_t)bytes * 10) >> 20;   // tenths of a MB
        unit = " MB";
    }
    append_unsigned(out, out_size, &pos, value / 10);
    if (pos + 1 < out_size) out[pos++] = '.';
    append_unsigned(out, out_size, &pos, value % 10);
    append_text(out, out_size, &pos, unit);
    out[pos] = 0;
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

        explorer_drive_labels[i][0] = 0;
        storage_device_info_t dev;
        const char* name = "Local Disk";
        if (fs_storage_get_device(i, &dev) && dev.label[0]) name = dev.label;
        append_string(explorer_drive_labels[i], sizeof(explorer_drive_labels[i]), name);

        char suffix[8];
        suffix[0] = ' ';
        suffix[1] = '(';
        suffix[2] = (char)('C' + i);
        suffix[3] = ':';
        suffix[4] = ')';
        suffix[5] = 0;
        append_string(explorer_drive_labels[i], sizeof(explorer_drive_labels[i]), suffix);
    }
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
    explorer_history_push(0);
    explorer_refresh();
}

static void explorer_navigate_to(const char* path) {
    if (!path) return;
    strncpy(explorer_path, path, sizeof(explorer_path) - 1);
    explorer_path[sizeof(explorer_path) - 1] = 0;
    explorer_this_pc_view = 0;
    explorer_history_push(0);
    explorer_selected = -1;
    explorer_refresh();
}

static const char* explorer_quick_path(int row) {
    switch (row) {
        case 0: return "C:/Users/Admin/Documents";
        case 1: return "C:/Users/Admin/Documents";
        case 2: return "C:/Users/Admin/Downloads";
        default: return 0;
    }
}

static const char* explorer_quick_label(int row) {
    static const char* labels[EXPLORER_SIDEBAR_ROWS] = {
        "Desktop", "Documents", "Downloads", "This PC"
    };
    if (row < 0 || row >= EXPLORER_SIDEBAR_ROWS) return "";
    return labels[row];
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
        "shell=GamerOS\nbuild=1.570\nsystem=C:/GamerOS/System32\napps=C:/GamerOS/Apps\nregistry=C:/GamerOS/Registry\n");
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
      write_text_file(settings_app_changelog_path(), settings_app_changelog_text());
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

// Compact UI text width in 8px units.
int compact_text_width(const char* str) {
    if (!str) return 0;
    int n = 0;
    while (str[n]) n++;
    return n * FONT_CW;
}

void draw_compact_string(int x, int y, const char* str, uint8_t color) {
    if (!str) return;
    while (*str) {
        uint8_t uc = (uint8_t)(*str);
        if (uc < 32 || uc > 127) uc = (uint8_t)' ';
        const uint8_t* bitmap = font_8x8[uc - 32];
        for (int row = 0; row < FONT_CH; row++) {
            uint8_t row_data = bitmap[row];
            for (int col = 0; col < FONT_CW; col++) {
                if (row_data & (0x80 >> col)) {
                    draw_pixel(x + col, y + row, color);
                }
            }
        }
        x += FONT_CW;
        str++;
    }
}

void draw_compact_string_clipped(int x, int y, int max_chars, const char* str, uint8_t color) {
    if (max_chars <= 0) return;
    int i = 0;
    while (str[i] && i < max_chars) {
        char ch[2];
        ch[0] = str[i];
        ch[1] = 0;
        draw_compact_string(x + (i * FONT_CW), y, ch, color);
        i++;
    }
}

void fill_chamfer_rect(int x, int y, int w, int h, uint8_t color) {
    if (w <= 0 || h <= 0) return;
    if (w < 3 || h < 3) {
        fill_rect(x, y, w, h, color);
        return;
    }
    fill_rect(x + 1, y, w - 2, h, color);
    fill_rect(x, y + 1, w, h - 2, color);
}

void draw_chamfer_rect(int x, int y, int w, int h, uint8_t color) {
    if (w <= 0 || h <= 0) return;
    if (w < 3 || h < 3) {
        draw_rect(x, y, w, h, color);
        return;
    }
    draw_rect(x + 1, y, w - 2, h, color);
    draw_rect(x, y + 1, w, h - 2, color);
}

static void draw_desktop_watermark(void) {
    const char* brand = "GamerOS 00m2";
    const char* tag = "Stability release";
    const char* build = "Build 1.570";

    int brand_w = compact_text_width(brand);
    int tag_w = compact_text_width(tag);
    int build_w = compact_text_width(build);

    const int accent_w = 4;
    const int pad_x = 8;
    const int gap = FONT_CW;

    int brand_row_w = brand_w;
    int meta_row_w = tag_w + gap + build_w;
    int inner_w = (brand_row_w > meta_row_w) ? brand_row_w : meta_row_w;

    int card_w = inner_w + pad_x * 2 + accent_w;
    int card_h = 26;
    int x = (int)current_width - card_w - 10;
    dock_layout_t wm_dock;
    dock_compute_layout(&wm_dock);
    int y = wm_dock.y - card_h - 10;
    if (settings_app_taskbar_style() == TASKBAR_STYLE_MACOS_DOCK) y += 6;
    if (x < 6) x = 6;
    if (y < 6) y = 6;

    fill_chamfer_rect(x, y, card_w, card_h, XP_WHITE);
    draw_chamfer_rect(x, y, card_w, card_h, XP_DGRAY);
    fill_rect(x + 2, y + 3, accent_w - 1, card_h - 6, XP_LBLUE);

    int tx = x + accent_w + pad_x;
    draw_compact_string(tx, y + 5, brand, XP_BLACK);
    draw_compact_string(tx, y + 15, tag, XP_DGRAY);
    draw_compact_string(x + card_w - pad_x - build_w, y + 15, build, XP_DGRAY);
}

static int wallpaper_is_too_dark(void) {
    static int cached = -1;
    static uint32_t last_w = 0;
    static uint32_t last_h = 0;
    if (cached >= 0 && last_w == background_wallpaper_width && last_h == background_wallpaper_height) {
        return cached;
    }
    last_w = background_wallpaper_width;
    last_h = background_wallpaper_height;
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

    // The embedded wallpaper samples as near-black; brighten if so.
    int too_dark = wallpaper_is_too_dark();

    if (graphics_is_truecolor()) {
        uint32_t w = current_width;
        uint32_t h = (uint32_t)desktop_h;

        for (uint32_t y = 0; y < h; y++) {
            uint32_t t = (y * 255) / (h > 1 ? h - 1 : 1);
            uint32_t r = 58 - ((40 * t) / 255);
            uint32_t g = 110 - ((55 * t) / 255);
            uint32_t b = 165 - ((30 * t) / 255);
            uint32_t rgb = (r << 16) | (g << 8) | b;
            if (too_dark) {
                r += 70; if (r > 255) r = 255;
                g += 70; if (g > 255) g = 255;
                b += 50; if (b > 255) b = 255;
                rgb = (r << 16) | (g << 8) | b;
            }
            uint32_t* row = graphics_get_rgb_row((int)y);
            if (!row) continue;
            for (uint32_t x = 0; x < w; x++) {
                row[x] = rgb;
            }
        }

        int band_y = (int)h * 2 / 5;
        int band_h = (int)h / 6;
        for (int y = band_y; y < band_y + band_h && y < (int)h; y++) {
            uint32_t t = (uint32_t)(((y - band_y) * 255) / (band_h > 1 ? band_h - 1 : 1));
            uint32_t a = t < 128 ? t : 255 - t;
            a = (a * 40) / 255;
            uint32_t* row = graphics_get_rgb_row(y);
            if (!row) continue;
            for (uint32_t x = 0; x < w; x++) {
                uint32_t px = row[x];
                uint32_t rr = ((px >> 16) & 0xFF) + a; if (rr > 255) rr = 255;
                uint32_t gg = ((px >> 8) & 0xFF) + a; if (gg > 255) gg = 255;
                uint32_t bb = (px & 0xFF) + a; if (bb > 255) bb = 255;
                row[x] = (rr << 16) | (gg << 8) | bb;
            }
        }

        int logo_x = 16;
        int logo_y = h - 68;
        if (logo_y < 16) logo_y = 16;
        // Semi-transparent brand bar
        for (int ly = 0; ly < 48; ly++) {
            int py = logo_y + ly;
            if (py < 0 || py >= (int)h) continue;
            uint32_t* row = graphics_get_rgb_row(py);
            if (!row) continue;
            for (int lx = 0; lx < 120; lx++) {
                int px = logo_x + lx;
                if (px < 0 || px >= (int)w) continue;
                uint32_t bg = row[px];
                uint32_t br = ((bg >> 16) & 0xFF) * 3 / 4 + 16;
                uint32_t bg_ = ((bg >> 8) & 0xFF) * 3 / 4 + 16;
                uint32_t bb = (bg & 0xFF) * 3 / 4 + 32;
                row[px] = (br << 16) | (bg_ << 8) | bb;
            }
        }
        return;
    }

    {
        for (int y = 0; y < desktop_h; y++) {
            uint8_t c;
            if (y < desktop_h / 4) {
                c = XP_BLUE;
            } else if (y < desktop_h * 3 / 4) {
                c = ((y & 1) == 0) ? XP_LBLUE : XP_WHITE;
            } else {
                c = XP_LGRAY;
            }
            fill_rect(0, y, (int)current_width, 1, c);
        }
        fill_rect(0, desktop_h * 2 / 5, (int)current_width, 3, XP_WHITE);
    }
}

void append_string(char* dest, size_t dest_cap, const char* src) {
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
    int item_count = 5;

    dock_layout_t dock;
    dock_compute_layout(&dock);

    int menu_x = dock.x;
    if (menu_x + menu_w > (int)current_width - 4) menu_x = (int)current_width - 4 - menu_w;
    if (menu_x < 4) menu_x = 4;

    if (menu_w > (int)current_width - 8) menu_w = (int)current_width - 8;
    int avail_h = dock.y - 4;
    if (avail_h < 40) avail_h = 40;
    if (menu_h > avail_h) menu_h = avail_h;

    m->x = menu_x;
    m->w = menu_w;
    m->h = menu_h;
    m->item_h = item_h;
    m->y = dock.y - menu_h;
    if (m->y < 0) m->y = 0;

    m->rail_w = START_MENU_RAIL_W;
    m->panel_x = m->x + m->rail_w + 8;
    m->panel_w = menu_w - m->rail_w - 14;
    if (m->panel_w < 8) m->panel_w = 8;
    m->items_top = m->y + START_MENU_ITEMS_TOP;
    m->item_count = item_count;
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
    if (menu_y + menu_h > (int)current_height - DOCK_RESERVED_H) menu_y = (int)current_height - DOCK_RESERVED_H - menu_h;
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
    int max_h = (int)current_height - DOCK_RESERVED_H - 8;
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
    if (debug_overlay_y > (int)current_height - DOCK_RESERVED_H - debug_overlay_h) {
        debug_overlay_y = (int)current_height - DOCK_RESERVED_H - debug_overlay_h;
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
        draw_compact_string_clipped(box_x + 10, box_y + 18 + i * 8, (box_w - 16) / FONT_CW, debug_log_lines[log_idx], XP_BLACK);
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
    if (win_type == WIN_NOTEPAD) {
        debug_log_message("App: load notepad data");
        debug_present_boot_status();
        notepad_app_init();
    }

    debug_log_message("App: launch complete");
    debug_present_boot_status();
    app_lifecycle_mark_launched(descriptor);
}

static void launch_settings_tab(int tab_idx) {
    launch_application_exe("SETTINGS.EXE", -1, -1);
    settings_app_select_tab(tab_idx);
}

static void startup_animation(void) {
    const char* title = "GamerOS";
    const char* subtitle = "Preparing your workspace...";
    const char* loading_label = "Loading";

    int panel_w = 380;
    if (panel_w > (int)current_width - 24) panel_w = (int)current_width - 24;
    int panel_h = 150;
    if (panel_h > (int)current_height - 24) panel_h = (int)current_height - 24;

    int panel_x = ((int)current_width - panel_w) / 2;
    int panel_y = ((int)current_height - panel_h) / 2;
    if (panel_x < 0) panel_x = 0;
    if (panel_y < 0) panel_y = 0;

    int rail_w = 46;
    int panel_inner_x = panel_x + rail_w + 12;
    int panel_inner_w = panel_w - rail_w - 24;
    if (panel_inner_w < 8) panel_inner_w = 8;

    int title_x = panel_inner_x;
    int title_y = panel_y + 12;

    int loader_cx = panel_x + (panel_w / 2);
    int loader_cy = panel_y + 88;
    int loading_w = compact_text_width(loading_label);
    int loading_x = loader_cx - (loading_w / 2);
    int loading_y = loader_cy + 26;

    int bg_h = (int)current_height;
    int frames = 48;

    if (graphics_is_truecolor()) {
        uint32_t w = current_width;
        uint32_t h = (uint32_t)bg_h;
        for (uint32_t y = 0; y < h; y++) {
            uint32_t t = (y * 255) / (h > 1 ? h - 1 : 1);
            uint32_t r = 20 + ((10 * t) / 255);
            uint32_t g = 50 + ((20 * t) / 255);
            uint32_t b = 90 + ((30 * t) / 255);
            uint32_t rgb = (r << 16) | (g << 8) | b;
            uint32_t* row = graphics_get_rgb_row((int)y);
            if (!row) continue;
            for (uint32_t x = 0; x < w; x++) row[x] = rgb;
        }
    } else {
        for (int y = 0; y < bg_h; y++) {
            uint8_t c;
            if (y < bg_h / 4) c = XP_BLUE;
            else if (y < bg_h * 3 / 4) c = ((y & 1) == 0) ? XP_LBLUE : XP_WHITE;
            else c = XP_LGRAY;
            fill_rect(0, y, (int)current_width, 1, c);
        }
    }

    fill_chamfer_rect(panel_x, panel_y, panel_w, panel_h, XP_WHITE);
    draw_chamfer_rect(panel_x, panel_y, panel_w, panel_h, XP_LGRAY);
    draw_chamfer_rect(panel_x + 1, panel_y + 1, panel_w - 2, panel_h - 2, XP_DGRAY);
    fill_chamfer_rect(panel_x + 4, panel_y + 4, rail_w, panel_h - 8, XP_LGRAY);
    fill_rect(panel_x + 5, panel_y + 5, rail_w - 2, 2, XP_WHITE);

    int chip_w = 24;
    int chip_x = panel_x + 4 + ((rail_w - chip_w) / 2);
    fill_chamfer_rect(chip_x, panel_y + 12, chip_w, 14, XP_LBLUE);
    draw_compact_string(chip_x + ((chip_w - FONT_CW) / 2), panel_y + 15, "G", XP_BLACK);

    fill_chamfer_rect(panel_inner_x, panel_y + 8, panel_inner_w, 18, XP_WHITE);
    fill_rect(panel_inner_x + 1, panel_y + 9, panel_inner_w - 2, 2, XP_LBLUE);
    draw_compact_string(title_x, title_y, title, XP_BLACK);

    fill_chamfer_rect(panel_inner_x, panel_y + 32, panel_inner_w, 14, XP_LGRAY);
    draw_chamfer_rect(panel_inner_x, panel_y + 32, panel_inner_w, 14, XP_DGRAY);
    draw_compact_string_clipped(panel_inner_x + 6, panel_y + 36,
                                (panel_inner_w - 10) / FONT_CW, subtitle, XP_BLACK);

    draw_compact_string(loading_x, loading_y, loading_label, XP_DGRAY);

    static const int8_t ring_dx[24] = {
         0,  5,  9, 13, 15, 17, 18, 17, 15, 13,  9,  5,
         0, -5, -9,-13,-15,-17,-18,-17,-15,-13, -9, -5
    };
    static const int8_t ring_dy[24] = {
       -18,-17,-15,-12, -9, -5,  0,  5,  9, 12, 15, 17,
        18, 17, 15, 12,  9,  5,  0, -5, -9,-12,-15,-17
    };

    int prev_lit = 0;
    for (int f = 0; f <= frames; f++) {
        int lit = ((f * 24) / frames);
        if (lit < 1) lit = 1;
        if (lit > 24) lit = 24;

        if (lit > prev_lit) {
            for (int i = prev_lit; i < lit; i++) {
                int px = loader_cx + ring_dx[i];
                int py = loader_cy + ring_dy[i];
                fill_rect(px - 2, py - 2, 5, 5, XP_BLUE);
            }
            if (lit > 0 && lit <= 24) {
                int px = loader_cx + ring_dx[lit - 1];
                int py = loader_cy + ring_dy[lit - 1];
                fill_rect(px - 2, py - 2, 5, 5, XP_LBLUE);
            }
            for (int i = lit; i < 24; i++) {
                int px = loader_cx + ring_dx[i];
                int py = loader_cy + ring_dy[i];
                fill_rect(px - 2, py - 2, 5, 5, XP_LGRAY);
            }
        }
        prev_lit = lit;

        swap_buffers();
        for (volatile int d = 0; d < 40000; d++) {
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

void init_windows(void) {
    for (int i = 0; i < MAX_WINDOWS; i++) {
        windows[i].active = 0;
    }
    notepad_app_init();
}

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
                strncpy(windows[i].title, notepad_app_title(), 31);
                windows[i].w = (int)current_width - 20;
                windows[i].h = (int)current_height - DOCK_RESERVED_H - 26;
                windows[i].x = 10;
                windows[i].y = 8;
                notepad_app_init();
            } else if (type == WIN_ABOUT) {
                strncpy(windows[i].title, "About GamerOS", 31);
                windows[i].w = 210;
                windows[i].h = 152;
            } else if (type == WIN_SETTINGS) {
                strncpy(windows[i].title, settings_app_title(), 31);
                windows[i].w = (int)current_width - 16;
                windows[i].h = (int)current_height - DOCK_RESERVED_H - 22;
                windows[i].x = 8;
                windows[i].y = 6;
                settings_app_reset_scroll();
            } else if (type == WIN_EXPLORER) {
                strncpy(windows[i].title, "File Explorer", 31);
                windows[i].w = (int)current_width - 16;
                windows[i].h = (int)current_height - DOCK_RESERVED_H - 22;
                windows[i].x = 8;
                windows[i].y = 6;
                explorer_open_this_pc();
                // Seed history at "This PC" so Back works immediately.
                explorer_history_reset();
                explorer_history_push(0);
            } else if (type == WIN_MYCOMP) {
                strncpy(windows[i].title, "My Computer", 31);
                windows[i].w = 180;
                windows[i].h = 140;
            }
            windows[i].title[31] = 0;
            if (windows[i].w > (int)current_width) windows[i].w = (int)current_width;
            if (windows[i].h > (int)current_height - DOCK_RESERVED_H) windows[i].h = (int)current_height - DOCK_RESERVED_H;
            if (windows[i].x < 0) windows[i].x = 0;
            if (windows[i].y < 0) windows[i].y = 0;
            if (windows[i].x > (int)current_width - windows[i].w) windows[i].x = (int)current_width - windows[i].w;
            if (windows[i].y > (int)current_height - DOCK_RESERVED_H - windows[i].h) {
                windows[i].y = (int)current_height - DOCK_RESERVED_H - windows[i].h;
            }
            window_count++;
            bring_window_to_front(i);
            return;
        }
    }
}

void close_window(int idx) {
    if (idx >= 0 && idx < MAX_WINDOWS && windows[idx].active) {
        if (windows[idx].type == WIN_NOTEPAD && notepad_app_is_dirty() && shell_storage_ready()) {
            notepad_app_save();
        }
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

void draw_tool_button(int x, int y, int w, int h, const char* label, int enabled) {
    uint8_t bg = enabled ? XP_WHITE : XP_LGRAY;
    uint8_t txt = enabled ? XP_BLACK : XP_DGRAY;
    fill_chamfer_rect(x, y, w, h, bg);
    draw_chamfer_rect(x, y, w, h, XP_DGRAY);
    if (enabled) fill_rect(x + 1, y + 1, w - 2, 1, XP_WHITE);
    int chars = (w - 6) / FONT_CW;
    draw_compact_string_clipped(x + 3, y + (h - 8) / 2, chars, label, txt);
}


void draw_window(window_t* win) {
    int x = win->x;
    int y = win->y;
    int w = win->w;
    int h = win->h;

    if (w < 24 || h < 24) return;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x >= (int)current_width || y >= (int)current_height) return;
    if (x + w > (int)current_width) w = (int)current_width - x;
    if (y + h > (int)current_height - DOCK_RESERVED_H) h = (int)current_height - DOCK_RESERVED_H - y;
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
    
    fill_chamfer_rect(x + w - 26, y + 4, 20, 16, is_active_window ? XP_WHITE : XP_LGRAY);
    draw_chamfer_rect(x + w - 26, y + 4, 20, 16, XP_DGRAY);
    draw_compact_string(x + w - 19, y + 8, "X", XP_BLACK);
    for (int g = 0; g < 4; g++) {
        int gx = x + w - 3 - (g * 3);
        int gy = y + h - 2;
        draw_line(gx, gy, x + w - 2, y + h - 3 - (g * 3), XP_DGRAY);
    }
    
    if (win->type == WIN_NOTEPAD) {
        notepad_app_draw_content(x + 2, y + 2, w - 4, h - 4, is_active_window);
    } else if (win->type == WIN_SETTINGS) {
        settings_app_draw_content(x + 4, y + 28, w - 8, h - 32, is_active_window);
    } else if (win->type == WIN_ABOUT) {
        fill_chamfer_rect(x + 4, y + 28, w - 8, h - 32, XP_WHITE);
        draw_chamfer_rect(x + 4, y + 28, w - 8, h - 32, XP_DGRAY);
        fill_chamfer_rect(x + 8, y + 32, w - 16, 22, XP_LGRAY);
        fill_rect(x + 9, y + 33, w - 18, 2, XP_LBLUE);
        draw_string(x + 14, y + 38, "About GamerOS", XP_BLACK);

        int content_x = x + 10;
        int content_y = y + 62;
        int content_w = w - 20;
        int content_chars = content_w / FONT_CW;
        if (content_chars < 1) content_chars = 1;
        char gfx_line[64];
        uint8_t bpp = graphics_get_bpp();
        sprintf(gfx_line, "Graphics: %ux%ux%u %s",
                (unsigned int)current_width,
                (unsigned int)current_height,
                (unsigned int)bpp,
                graphics_is_truecolor() ? "RGBA" : "Indexed");
        draw_compact_string_clipped(content_x, content_y + 0, content_chars, "Version: 00m2", XP_BLACK);
        draw_compact_string_clipped(content_x, content_y + 12, content_chars, "Build: 1.570", XP_BLACK);
        draw_compact_string_clipped(content_x, content_y + 24, content_chars, "Design language: GamerOS Modern", XP_BLACK);
        draw_compact_string_clipped(content_x, content_y + 42, content_chars, "System", XP_DGRAY);
        draw_compact_string_clipped(content_x, content_y + 54, content_chars, "Kernel: x86_64", XP_BLACK);
        draw_compact_string_clipped(content_x, content_y + 66, content_chars, gfx_line, XP_BLACK);
        draw_compact_string_clipped(content_x, content_y + 78, content_chars, "System32: C:/GamerOS/System32", XP_BLACK);
        draw_compact_string_clipped(content_x, content_y + 96, content_chars, "Apps", XP_DGRAY);
        draw_compact_string_clipped(content_x, content_y + 108, content_chars, "Apps: *.EXE (NOTEPAD/SETTINGS/EXPLORER/ABOUT)", XP_BLACK);
        draw_compact_string_clipped(content_x, content_y + 120, content_chars, "Author: Chosentechies", XP_BLACK);
    } else if (win->type == WIN_EXPLORER) {
        // ---- Layout ----
        const int TB_H = 14;
        const int BTN_W = 26;
        const int BTN_GAP = 3;
        const int SIDEBAR_W = 88;
        const int ROW_H = 12;
        const int STATUS_H = 12;

        int cy = y + 26;
        int cb = y + h - 3;
        int tb_y = cy + 2;
        int body_y = tb_y + TB_H + 3;
        int status_y = cb - STATUS_H;
        int body_h = status_y - body_y - 3;
        if (body_h < ROW_H) body_h = ROW_H;

        // ---- Client background ----
        fill_chamfer_rect(x + 4, cy, w - 8, cb - cy, XP_LGRAY);
        draw_chamfer_rect(x + 4, cy, w - 8, cb - cy, XP_DGRAY);

        // ---- Toolbar ----
        int bx = x + 7;
        static const char* tb_labels[EXPLORER_TB_BUTTONS] = { "<", ">", "^", "R" };
        explorer_tb_y = tb_y;
        explorer_tb_w = BTN_W;
        explorer_tb_h = 13;
        for (int i = 0; i < EXPLORER_TB_BUTTONS; i++) {
            int enabled;
            if (i == EXPLORER_TB_BACK) enabled = explorer_can_go_back();
            else if (i == EXPLORER_TB_FWD) enabled = explorer_can_go_forward();
            else if (i == EXPLORER_TB_UP) enabled = !explorer_this_pc_view && strcmp(explorer_path, "C:/") != 0;
            else enabled = 1;
            explorer_tb_enabled[i] = enabled;
            explorer_tb_x[i] = bx;
            draw_tool_button(bx, tb_y, BTN_W, 13, tb_labels[i], enabled);
            bx += BTN_W + BTN_GAP;
        }

        // ---- Breadcrumb ----
        {
            int bc_x = bx + 4;
            int bc_w = (x + w - 6) - bc_x;
            if (bc_w < 24) bc_w = 24;
            fill_chamfer_rect(bc_x, tb_y, bc_w, 13, XP_WHITE);
            draw_chamfer_rect(bc_x, tb_y, bc_w, 13, XP_DGRAY);
            const char* crumb = explorer_this_pc_view ? "This PC" : explorer_path;
            int cc = (bc_w - 8) / FONT_CW;
            if (cc < 4) cc = 4;
            draw_compact_string_clipped(bc_x + 4, tb_y + 3, cc, crumb, XP_BLACK);
        }

        // ---- Sidebar ----
        int sb_x = x + 7;
        int sb_h = body_h;
        fill_chamfer_rect(sb_x, body_y, SIDEBAR_W, sb_h, XP_WHITE);
        draw_chamfer_rect(sb_x, body_y, SIDEBAR_W, sb_h, XP_DGRAY);

        explorer_sb_x = sb_x;
        explorer_sb_w = SIDEBAR_W;
        explorer_sb_hit_h = ROW_H;
        explorer_sb_selected = -1;

        draw_compact_string(sb_x + 5, body_y + 3, "Quick Access", XP_BLUE);
        fill_rect(sb_x + 3, body_y + 11, SIDEBAR_W - 6, 1, XP_LGRAY);

        int sb_first = body_y + 15;
        for (int i = 0; i < EXPLORER_SIDEBAR_ROWS; i++) {
            int ry = sb_first + i * ROW_H;
            if (ry + ROW_H > body_y + sb_h) break;
            explorer_sb_hit_y[i] = ry;

            int active = 0;
            if (i == 3) {
                active = explorer_this_pc_view;
            } else if (!explorer_this_pc_view && explorer_quick_path(i) &&
                       strcmp(explorer_path, explorer_quick_path(i)) == 0) {
                active = 1;
            }
            if (active) explorer_sb_selected = i;

            if (active) {
                fill_chamfer_rect(sb_x + 2, ry, SIDEBAR_W - 4, ROW_H - 1, XP_LBLUE);
            }
            fill_rect(sb_x + 6, ry + 4, 4, 3, active ? XP_WHITE : XP_LBLUE);
            draw_compact_string_clipped(sb_x + 14, ry + 2, (SIDEBAR_W - 18) / FONT_CW,
                                        explorer_quick_label(i), XP_BLACK);
        }

        // ---- Entry list ----
        int list_x = sb_x + SIDEBAR_W + 5;
        int list_w = (x + w - 6) - list_x;
        if (list_w < 20) list_w = 20;
        fill_chamfer_rect(list_x, body_y, list_w, body_h, XP_WHITE);
        draw_chamfer_rect(list_x, body_y, list_w, body_h, XP_DGRAY);

        // ---- Details view columns ----
        int show_wide_cols = 1;
        int col_size_x = list_x + list_w - EXPL_COL_SIZE_W - 4;
        int col_type_x = col_size_x - EXPL_COL_TYPE_W;
        int col_date_x = col_type_x - EXPL_COL_DATE_W;
        if (col_date_x < list_x + 90) {
            show_wide_cols = 0;
            col_date_x = col_size_x - EXPL_COL_TYPE_W;
            if (col_date_x < list_x + 90) col_date_x = list_x + 90;
        }
        int name_col_w = (show_wide_cols ? col_date_x : col_type_x) - (list_x + 15) - 8;

        fill_rect(list_x + 2, body_y + 2, list_w - 4, 9, XP_LGRAY);
        draw_compact_string_clipped(list_x + 6, body_y + 3,
                                    (name_col_w > 16 ? name_col_w : 16) / FONT_CW,
                                    "Name", XP_BLACK);
        if (show_wide_cols) {
            draw_compact_string_clipped(col_date_x, body_y + 3,
                                        (EXPL_COL_DATE_W - 8) / FONT_CW, "Date modified", XP_BLACK);
            draw_compact_string_clipped(col_type_x, body_y + 3,
                                        (EXPL_COL_TYPE_W - 8) / FONT_CW, "Type", XP_BLACK);
            fill_rect(col_date_x - 5, body_y + 2, 1, 9, XP_DGRAY);
            fill_rect(col_type_x - 5, body_y + 2, 1, 9, XP_DGRAY);
        } else {
            draw_compact_string_clipped(col_date_x, body_y + 3,
                                        (EXPL_COL_TYPE_W - 8) / FONT_CW, "Type", XP_BLACK);
            fill_rect(col_date_x - 5, body_y + 2, 1, 9, XP_DGRAY);
        }
        draw_compact_string_clipped(col_size_x, body_y + 3,
                                    (EXPL_COL_SIZE_W - 8) / FONT_CW, "Size", XP_BLACK);
        fill_rect(col_size_x - 5, body_y + 2, 1, 9, XP_DGRAY);
        fill_rect(list_x + 2, body_y + 11, list_w - 4, 1, XP_DGRAY);

        explorer_list_x = list_x;
        explorer_list_y = body_y + 14;
        explorer_list_w = list_w;
        explorer_list_hit_h = ROW_H;
        explorer_list_hit_count = 0;

        int row_y0 = body_y + 14;
        int name_chars = (list_w - 24) / FONT_CW;
        if (name_chars < 6) name_chars = 6;
        int row_pitch = explorer_this_pc_view ? EXPLORER_DRIVE_ROW_H : ROW_H;
        explorer_list_hit_h = row_pitch;
        int rows_visible = (body_h - 18) / row_pitch;
        if (rows_visible < 1) rows_visible = 1;

        if (explorer_this_pc_view) {
            for (int i = 0; i < explorer_drive_count && i < rows_visible; i++) {
                int ry = row_y0 + i * EXPLORER_DRIVE_ROW_H;
                if (ry + EXPLORER_DRIVE_ROW_H > body_y + body_h) break;

                if (explorer_selected == i) {
                    fill_chamfer_rect(list_x + 2, ry, list_w - 4, EXPLORER_DRIVE_ROW_H - 2, XP_LBLUE);
                }

                fill_rect(list_x + 6, ry + 3, 7, 6, XP_DGRAY);
                fill_rect(list_x + 6, ry + 3, 7, 2, XP_WHITE);

                draw_compact_string_clipped(list_x + 18, ry + 2, name_chars,
                                            explorer_drive_labels[i], XP_BLACK);

                fs_space_info_t space;
                storage_device_info_t dev;
                int bar_x = list_x + 18;
                int bar_w = list_w - 30;
                if (bar_w < 20) bar_w = 20;
                int bar_y = ry + 13;

                if (fs_storage_get_device(i, &dev) && bar_w > 0) {
                    uint64_t total = dev.total_bytes;
                    uint64_t used = dev.used_bytes;
                    if (used > total) used = total;
                    int used_px = (total > 0) ? (int)((used * bar_w) / total) : 0;
                    if (used_px < 0) used_px = 0;
                    if (used_px > bar_w) used_px = bar_w;

                    fill_rect(bar_x, bar_y, bar_w, EXPLORER_BAR_H, XP_WHITE);
                    draw_chamfer_rect(bar_x, bar_y, bar_w, EXPLORER_BAR_H, XP_DGRAY);
                    if (used_px > 0) {
                        fill_rect(bar_x + 1, bar_y + 1, used_px - 1, EXPLORER_BAR_H - 2, XP_BLUE);
                    }
                }

                if (fs_get_space_info(&space)) {
                    char line[64];
                    format_capacity_line(line, (int)sizeof(line), space.free_bytes, space.total_bytes);
                    draw_compact_string_clipped(bar_x, bar_y + EXPLORER_BAR_H + 4,
                                                (bar_w + 4) / FONT_CW, line, XP_DGRAY);
                }

                explorer_list_hit_y[explorer_list_hit_count++] = ry;
            }
        } else {
            for (int i = 0; i < explorer_entry_count && i < rows_visible; i++) {
                int ry = row_y0 + i * ROW_H;
                if (ry + ROW_H > body_y + body_h) break;
                if (explorer_selected == i) {
                    fill_chamfer_rect(list_x + 2, ry, list_w - 4, ROW_H - 1, XP_LBLUE);
                }
                if (explorer_is_dir[i]) {
                    fill_rect(list_x + 6, ry + 4, 5, 4, XP_LBLUE);
                } else {
                    fill_rect(list_x + 6, ry + 2, 5, 7, XP_DGRAY);
                    fill_rect(list_x + 6, ry + 2, 5, 2, XP_WHITE);
                }

                const char* full_path = explorer_entries[i];
                const char* leaf = path_basename(full_path);
                draw_compact_string_clipped(list_x + 15, ry + 2,
                                            (name_col_w > 8 ? name_col_w : 8) / FONT_CW,
                                            leaf, XP_BLACK);

                uint32_t entry_size = 0;
                uint32_t entry_mtime = 0;
                fs_get_entry_info(full_path, explorer_is_dir[i], &entry_size, &entry_mtime);

                char cell[64];
                if (show_wide_cols) {
                    format_mtime(cell, (int)sizeof(cell), entry_mtime);
                    draw_compact_string_clipped(col_date_x, ry + 2,
                                                (EXPL_COL_DATE_W - 8) / FONT_CW, cell, XP_DGRAY);
                }
                draw_compact_string_clipped(col_type_x, ry + 2,
                                            (EXPL_COL_TYPE_W - 8) / FONT_CW,
                                            file_type_label(full_path, explorer_is_dir[i]), XP_DGRAY);
                if (!explorer_is_dir[i]) {
                    format_file_size(cell, (int)sizeof(cell), entry_size);
                } else {
                    cell[0] = 0;
                }
                draw_compact_string_clipped(col_size_x, ry + 2,
                                            (EXPL_COL_SIZE_W - 8) / FONT_CW, cell, XP_DGRAY);
                explorer_list_hit_y[explorer_list_hit_count++] = ry;
            }
            if (explorer_entry_count == 0) {
                draw_compact_string_clipped(list_x + 15, row_y0 + 2, name_chars,
                                            "This folder is empty", XP_DGRAY);
            }
        }

        // ---- Status bar ----
        fill_rect(x + 7, status_y, w - 14, STATUS_H - 1, XP_LGRAY);
        fill_rect(x + 8, status_y + 1, w - 16, 1, XP_WHITE);
        {
            char count_line[48];
            int n = explorer_this_pc_view ? explorer_drive_count : explorer_entry_count;
            sprintf(count_line, "%d item%s", n, n == 1 ? "" : "s");
            draw_compact_string_clipped(x + 11, status_y + 2, (w - 70) / FONT_CW, count_line, XP_DGRAY);

            const char* right = explorer_this_pc_view ? "Computer" : path_basename(explorer_path);
            int rw = (int)strlen(right) * FONT_CW;
            if (rw < w - 90) {
                draw_compact_string(x + w - 10 - rw, status_y + 2, right, XP_DGRAY);
            }
        }
    }
}

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
        settings_app_draw_icon(x, y);
        return;
    } else if (window_type == WIN_EXPLORER) {
        draw_explorer_icon(x, y);
        return;
    }
    notepad_app_draw_icon(x, y);
}

static void draw_start_menu(void) {
    start_menu_metrics_t sm = {0};
    get_start_menu_metrics(&sm);
    int menu_x = sm.x;
    int menu_w = sm.w;
    int menu_h = sm.h;
    int menu_y = sm.y;
    int item_h = sm.item_h;
    int rail_w = sm.rail_w;
    int panel_x = sm.panel_x;
    int panel_w = sm.panel_w;
    int items_top = sm.items_top;

    fill_chamfer_rect(menu_x, menu_y, menu_w, menu_h, XP_WHITE);
    draw_chamfer_rect(menu_x, menu_y, menu_w, menu_h, XP_LGRAY);
    draw_chamfer_rect(menu_x + 1, menu_y + 1, menu_w - 2, menu_h - 2, XP_DGRAY);

    fill_chamfer_rect(menu_x + 4, menu_y + 4, rail_w, menu_h - 8, XP_LGRAY);
    fill_rect(menu_x + 5, menu_y + 5, rail_w - 2, 2, XP_LBLUE);

    int rail_cx = menu_x + 4 + (rail_w / 2);
    int chip_w = 28;
    int chip_x = rail_cx - (chip_w / 2);
    fill_chamfer_rect(chip_x, menu_y + 12, chip_w, 16, XP_WHITE);
    draw_chamfer_rect(chip_x, menu_y + 12, chip_w, 16, XP_DGRAY);
    draw_compact_string(chip_x + ((chip_w - FONT_CW) / 2), menu_y + 16, "A", XP_BLACK);

    int rail_label_chars = (rail_w - 8) / FONT_CW;
    draw_compact_string_clipped(rail_cx - (compact_text_width("Admin") / 2), menu_y + 36,
                                rail_label_chars, "Admin", XP_BLACK);
    draw_compact_string_clipped(rail_cx - (compact_text_width("Profile") / 2), menu_y + 47,
                                rail_label_chars, "Profile", XP_DGRAY);

    fill_chamfer_rect(panel_x, menu_y + 4, panel_w, 20, XP_WHITE);
    fill_rect(panel_x + 1, menu_y + 5, panel_w - 2, 2, XP_LBLUE);
    draw_compact_string(panel_x + 6, menu_y + 10, "GamerOS", XP_BLACK);
    int header_chars = (panel_w - 12) / FONT_CW;
    draw_compact_string_clipped(panel_x + 6 + (compact_text_width("GamerOS") + FONT_CW),
                                menu_y + 11, header_chars, "Launchpad", XP_DGRAY);

    fill_chamfer_rect(panel_x, menu_y + 28, panel_w, 14, XP_LGRAY);
    draw_chamfer_rect(panel_x, menu_y + 28, panel_w, 14, XP_DGRAY);
    draw_compact_string_clipped(panel_x + 6, menu_y + 32, header_chars, "Search apps", XP_DGRAY);

    static const char* items[] = {
        "Notepad",
        "Settings",
        "File Explorer",
        "GamerOS Update",
        "About GamerOS"
    };
    int item_count = sm.item_count;
    int text_chars = (panel_w - 24) / FONT_CW;
    if (text_chars < 1) text_chars = 1;
    for (int i = 0; i < item_count; i++) {
        int iy = items_top + i * item_h;
        fill_chamfer_rect(panel_x, iy, panel_w, item_h - 2, (i == 1) ? XP_LBLUE : XP_WHITE);
        draw_chamfer_rect(panel_x, iy, panel_w, item_h - 2, XP_DGRAY);
        fill_chamfer_rect(panel_x + 6, iy + 4, 10, 10, XP_LGRAY);
        draw_chamfer_rect(panel_x + 6, iy + 4, 10, 10, XP_DGRAY);
        draw_compact_string_clipped(panel_x + 22, iy + 6, text_chars, items[i], XP_BLACK);
    }

    int shutdown_y = menu_y + menu_h - 24;
    fill_chamfer_rect(panel_x, shutdown_y, panel_w, 16, XP_LGRAY);
    draw_chamfer_rect(panel_x, shutdown_y, panel_w, 16, XP_DGRAY);
    fill_rect(panel_x + 1, shutdown_y + 1, panel_w - 2, 2, XP_LBLUE);
    draw_compact_string(panel_x + 8, shutdown_y + 4, "Shut Down", XP_BLACK);
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
    int y = (((int)current_height - DOCK_RESERVED_H) - h) / 2;
    if (x < 4) x = 4;
    if (y < 4) y = 4;

    fill_chamfer_rect(x, y, w, h, XP_WHITE);
    draw_chamfer_rect(x, y, w, h, XP_DGRAY);
    fill_chamfer_rect(x + 2, y + 2, w - 4, 16, XP_LBLUE);
    draw_compact_string(x + 8, y + 6, error_popup_title, XP_BLACK);

    draw_compact_string_clipped(x + 8, y + 26, (w - 16) / FONT_CW, error_popup_message, XP_BLACK);
    draw_compact_string_clipped(x + 8, y + 38, (w - 16) / FONT_CW, "Check serial log for details.", XP_DGRAY);

    fill_chamfer_rect(x + w - 58, y + h - 22, 46, 14, XP_WHITE);
    draw_chamfer_rect(x + w - 58, y + h - 22, 46, 14, XP_DGRAY);
    draw_compact_string(x + w - 44, y + h - 18, "OK", XP_BLACK);
}

void draw_desktop(void) {
    layout_desktop_icons();
    draw_desktop_wallpaper((int)current_height);
    draw_desktop_watermark();
    
    for (int i = 0; i < (int)NUM_ICONS; i++) {
        int x = desktop_icons[i].x;
        int y = desktop_icons[i].y;
        draw_desktop_icon(x, y, desktop_icons[i].window_type);
        int label_w = (int)strlen(desktop_icons[i].label) * FONT_CW;
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
    
    dock_layout_t dock;
    dock_compute_layout(&dock);

    if (settings_app_taskbar_style() == TASKBAR_STYLE_WINDOWS) {
        fill_chamfer_rect(dock.x, dock.y, dock.w, dock.h, XP_LGRAY);
        draw_chamfer_rect(dock.x, dock.y, dock.w, dock.h, XP_DGRAY);
        fill_rect(dock.x + 1, dock.y + 1, dock.w - 2, 1, XP_WHITE);
    } else {
        for (int s = DOCK_SHADOW; s >= 1; s--) {
            vga_draw_rounded_rect(dock.x - s, dock.y + s, (uint32_t)(dock.w + (s * 2)),
                                  (uint32_t)dock.h, (uint8_t)(DOCK_RADIUS + s), XP_SHADOW);
        }
        vga_fill_rounded_rect(dock.x, dock.y, (uint32_t)dock.w, (uint32_t)dock.h,
                              (uint8_t)DOCK_RADIUS, XP_LGRAY);
        vga_draw_rounded_rect(dock.x, dock.y, (uint32_t)dock.w, (uint32_t)dock.h,
                              (uint8_t)DOCK_RADIUS, XP_DGRAY);
        fill_rect(dock.x + DOCK_RADIUS, dock.y + 1, dock.w - (DOCK_RADIUS * 2), 1, XP_WHITE);
    }

    const char* start_label = "Start";
    const int start_flag_w = 12;
    fill_chamfer_rect(dock.start_x, dock.start_y, START_BTN_W, START_BTN_H,
                      start_menu_open ? XP_LBLUE : XP_WHITE);
    fill_rect(dock.start_x + 1, dock.start_y + 1, START_BTN_W - 2, 2, XP_LBLUE);
    fill_rect(dock.start_x + 5, dock.start_y + 4, 4, 8, XP_LBLUE);
    draw_chamfer_rect(dock.start_x, dock.start_y, START_BTN_W, START_BTN_H, XP_DGRAY);
    {
        int start_text_w = compact_text_width(start_label);
        int start_text_area_x = dock.start_x + start_flag_w;
        int start_text_area_w = START_BTN_W - start_flag_w - 4;
        int start_text_x = start_text_area_x + ((start_text_area_w - start_text_w) / 2);
        int start_text_y = dock.start_y + ((START_BTN_H - FONT_CH) / 2);
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

    int taskbtn_w = dock.taskbtn_w;
    int taskbtn_y = dock.taskbtn_y;
    int task_x = dock.task_x;

    int task_chars = (taskbtn_w - 8) / FONT_CW;
    if (task_chars < 1) task_chars = 1;
    if (task_chars > 12) task_chars = 12;

    int drawn = 0;
    for (int i = 0; i < MAX_WINDOWS && drawn < dock.task_count; i++) {
        if (windows[i].active) {
            if (i == active_window) {
                fill_chamfer_rect(task_x, taskbtn_y, taskbtn_w, TASKBTN_H, XP_LBLUE);
            } else {
                fill_chamfer_rect(task_x, taskbtn_y, taskbtn_w, TASKBTN_H, XP_WHITE);
            }
            draw_chamfer_rect(task_x, taskbtn_y, taskbtn_w, TASKBTN_H, XP_DGRAY);
            if (i == active_window) {
                fill_rect(task_x + 1, taskbtn_y + 1, taskbtn_w - 2, 2, XP_WHITE);
            }

            size_t len = strlen(windows[i].title);
            if ((int)len > task_chars) len = (size_t)task_chars;
            char task_text[16];
            if (len > 0) {
                strncpy(task_text, windows[i].title, len);
            }
            task_text[len] = 0;
            int tx = task_x + ((taskbtn_w - ((int)len * FONT_CW)) / 2);
            int ty = taskbtn_y + ((TASKBTN_H - FONT_CH) / 2);
            draw_compact_string(tx, ty, task_text, XP_BLACK);
            task_x += taskbtn_w + 2;
            drawn++;
        }
    }
    
    uint8_t hour = 0, minute = 0, second = 0;
    uint8_t day = 0, month = 0;
  uint16_t year = 0;
  // Taskbar clock shows local time for the selected zone, not raw RTC time.
  {
      uint32_t utc_now = get_unix_time();
      tz_local_time(utc_now, &year, &month, &day, &hour, &minute, &second);
  }
  char datetime_str[20];
    datetime_str[0] = (char)('0' + ((hour / 10) % 10));
    datetime_str[1] = (char)('0' + (hour % 10));
    datetime_str[2] = ':';
    datetime_str[3] = (char)('0' + ((minute / 10) % 10));
    datetime_str[4] = (char)('0' + (minute % 10));
    int datetime_len = 5;
    if (settings_app_show_seconds()) {
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
    fill_chamfer_rect(dock.clock_x, dock.clock_y, dock.clock_w, TASKBTN_H, XP_WHITE);
    draw_chamfer_rect(dock.clock_x, dock.clock_y, dock.clock_w, TASKBTN_H, XP_DGRAY);
    fill_rect(dock.clock_x + 1, dock.clock_y + 1, dock.clock_w - 2, 2, XP_LBLUE);
    {
        int clock_text_w = ((int)strlen(datetime_str)) * FONT_CW;
        int clock_text_x = dock.clock_x + ((dock.clock_w - clock_text_w) / 2);
        int clock_text_y = dock.clock_y + ((TASKBTN_H - FONT_CH) / 2);
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


static int handle_explorer_click(window_t* win, int32_t mx, int32_t my) {
    if (!win) return 0;

    // ---- Toolbar buttons ----
    if (explorer_tb_h > 0 && my >= explorer_tb_y && my < explorer_tb_y + explorer_tb_h) {
        for (int i = 0; i < EXPLORER_TB_BUTTONS; i++) {
            if (mx < explorer_tb_x[i] || mx >= explorer_tb_x[i] + explorer_tb_w) continue;
            if (!explorer_tb_enabled[i]) return 1;
            if (i == EXPLORER_TB_BACK) explorer_history_step(-1);
            else if (i == EXPLORER_TB_FWD) explorer_history_step(1);
            else if (i == EXPLORER_TB_UP) explorer_go_parent();
            else { explorer_selected = -1; explorer_refresh(); }
            return 1;
        }
    }

    // ---- Quick access sidebar ----
    if (explorer_sb_hit_h > 0 &&
        mx >= explorer_sb_x && mx < explorer_sb_x + explorer_sb_w) {
        for (int i = 0; i < EXPLORER_SIDEBAR_ROWS; i++) {
            int ry = explorer_sb_hit_y[i];
            if (ry <= 0) continue;
            if (my < ry || my >= ry + explorer_sb_hit_h) continue;
            if (i == 3) {
                if (!storage_initialized) {
                    ensure_storage_initialized();
                    explorer_build_this_pc_entries();
                }
                explorer_open_this_pc();
                explorer_history_push(1);
            } else {
                const char* dest = explorer_quick_path(i);
                if (dest) explorer_navigate_to(dest);
            }
            return 1;
        }
    }

    // ---- Entry list ----
    if (explorer_list_hit_h > 0 &&
        mx >= explorer_list_x && mx < explorer_list_x + explorer_list_w) {
        for (int i = 0; i < explorer_list_hit_count; i++) {
            int ry = explorer_list_hit_y[i];
            if (my < ry || my >= ry + explorer_list_hit_h) continue;

            if (explorer_this_pc_view) {
                if (!storage_initialized) {
                    ensure_storage_initialized();
                    explorer_build_this_pc_entries();
                }
                if (i >= explorer_drive_count) return 1;
                explorer_selected = i;
                strncpy(explorer_path, explorer_drive_paths[i], sizeof(explorer_path) - 1);
                explorer_path[sizeof(explorer_path) - 1] = 0;
                explorer_this_pc_view = 0;
                explorer_history_push(1);
                explorer_refresh();
                return 1;
            }

            if (i >= explorer_entry_count) return 1;
            explorer_selected = i;
            if (explorer_is_dir[i]) {
                explorer_navigate_to(explorer_entries[i]);
            } else if (path_has_extension(explorer_entries[i], ".EXE")) {
                launch_application_exe(explorer_entries[i], -1, -1);
            } else if (path_has_extension(explorer_entries[i], ".gosapp")) {
                raise_runtime_error("Manifest Info", path_basename(explorer_entries[i]));
            }
            return 1;
        }
    }
    return 0;
}

void process_mouse(int32_t mx, int32_t my, uint8_t buttons, int8_t wheel_delta) {
#define RETURN_MOUSE() do { last_buttons = buttons; return; } while (0)
    if (mx < 0) mx = 0;
    if (my < 0) my = 0;
    if (mx >= (int32_t)current_width) mx = (int32_t)current_width - 1;
    if (my >= (int32_t)current_height) my = (int32_t)current_height - 1;
    if (mx < 0 || my < 0) RETURN_MOUSE();

    buttons &= (uint8_t)(MOUSE_BTN_LEFT | MOUSE_BTN_RIGHT);
    if ((buttons & (MOUSE_BTN_LEFT | MOUSE_BTN_RIGHT)) == (MOUSE_BTN_LEFT | MOUSE_BTN_RIGHT)) {
        // Ignore ambiguous dual-button packets (unstable VM popup paths).
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
        int desktop_h = (int)current_height - DOCK_RESERVED_H;
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
            int ey = (((int)current_height - DOCK_RESERVED_H) - eh) / 2;
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

        dock_layout_t dock;
        dock_compute_layout(&dock);

        if (mx >= dock.start_x && mx < dock.start_x + START_BTN_W &&
            my >= dock.start_y && my < dock.start_y + START_BTN_H) {
            start_menu_open = !start_menu_open;
            desktop_menu_open = 0;
            RETURN_MOUSE();
        }

        if (start_menu_open) {
            start_menu_metrics_t sm = {0};
            get_start_menu_metrics(&sm);
            int menu_x = sm.x;
            int menu_w = sm.w;
            int menu_h = sm.h;
            int menu_y = sm.y;
            int menu_item_h = sm.item_h;
            int panel_x = sm.panel_x;
            int panel_w = sm.panel_w;
            int items_top = sm.items_top;
            if (mx >= menu_x && mx < menu_x + menu_w && my >= menu_y && my < menu_y + menu_h) {
                if (mx >= panel_x && mx < panel_x + panel_w &&
                    my >= items_top && my < items_top + menu_item_h * sm.item_count) {
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
            if (active_window < 0 || active_window >= MAX_WINDOWS) {
                RETURN_MOUSE();
            }
            window_t* focused = &windows[active_window];
            if (focused->type == WIN_SETTINGS && settings_app_handle_click(mx, my)) {
                RETURN_MOUSE();
            }
            if (focused->type == WIN_EXPLORER && handle_explorer_click(focused, mx, my)) {
                RETURN_MOUSE();
            }
            if (focused->type == WIN_NOTEPAD && notepad_app_handle_click(mx, my)) {
                RETURN_MOUSE();
            }
            RETURN_MOUSE();
        }
        
        for (int i = 0; i < (int)NUM_ICONS; i++) {
            int ix = desktop_icons[i].x;
            int iy = desktop_icons[i].y;
            if (mx >= ix && mx < ix + DESKTOP_ICON_HIT_W &&
                my >= iy && my < iy + DESKTOP_ICON_HIT_H) {
                launch_application_exe(desktop_icons[i].exe_name, 60 + i * 30, 40 + i * 20);
                RETURN_MOUSE();
            }
        }
        
        {
            int taskbtn_y = dock.taskbtn_y;
            int taskbtn_w = dock.taskbtn_w;
            int task_x = dock.task_x;
            int matched = 0;
            for (int i = 0; i < MAX_WINDOWS && matched < dock.task_count; i++) {
                if (windows[i].active) {
                    if (mx >= task_x && mx < task_x + taskbtn_w &&
                        my >= taskbtn_y &&
                        my < taskbtn_y + TASKBTN_H) {
                        bring_window_to_front(i);
                        RETURN_MOUSE();
                    }
                    task_x += taskbtn_w + 2;
                    matched++;
                }
            }
        }
    }
    
    if (buttons & MOUSE_BTN_LEFT) {
        if (debug_overlay_enabled && debug_overlay_dragging) {
            debug_overlay_x = mx - debug_overlay_drag_x;
            debug_overlay_y = my - debug_overlay_drag_y;
        } else if (debug_overlay_enabled && debug_overlay_resizing) {
            int min_w = 160;
            int min_h = 42;
            int max_w = (int)current_width - debug_overlay_x;
            int max_h = ((int)current_height - DOCK_RESERVED_H) - debug_overlay_y;
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
                if (windows[i].x > (int)current_width - windows[i].w) {
                    windows[i].x = (int)current_width - windows[i].w;
                }
                if (windows[i].x < 0) windows[i].x = 0;
                if (windows[i].y > (int)current_height - DOCK_RESERVED_H - 20) {
                    windows[i].y = (int)current_height - DOCK_RESERVED_H - 20;
                }
                if (windows[i].y < 0) windows[i].y = 0;
            } else if (windows[i].active && windows[i].resizing) {
                int min_w = WINDOW_MIN_W_DEFAULT;
                int min_h = WINDOW_MIN_H_DEFAULT;
                get_window_min_size(&windows[i], &min_w, &min_h);
                int max_w = (int)current_width - windows[i].x;
                int max_h = ((int)current_height - DOCK_RESERVED_H) - windows[i].y;
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
                (void)notepad_app_handle_wheel(wheel_delta);
            } else if (hovered->type == WIN_SETTINGS) {
                (void)settings_app_handle_wheel(mx, my, wheel_delta);
            }
        }
    }
    
    last_buttons = buttons;
#undef RETURN_MOUSE
}

int process_keyboard(void) {
    if (active_window < 0 || !windows[active_window].active) return 0;
    
    window_t* win = &windows[active_window];
    int changed = 0;
    
    while (1) {
        char c = keyboard_getchar();
        if (!c) break;

        if (win->type == WIN_NOTEPAD) {
            changed |= notepad_app_handle_key(c);
            continue;
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

// ---- Shell API ------------------------------------------------------------

void shell_ensure_storage_initialized(void) {
    ensure_storage_initialized();
}

uint8_t shell_storage_ready(void) {
    return storage_initialized;
}

file_t* shell_fs_open(const char* gos_path) {
    if (!storage_initialized) ensure_storage_initialized();
    return gos_fs_open_file(gos_path);
}

file_t* shell_fs_create(const char* gos_path) {
    if (!storage_initialized) ensure_storage_initialized();
    return gos_fs_create_file(gos_path);
}

void shell_raise_error(const char* title, const char* message) {
    raise_runtime_error(title, message);
}

// ---- App configuration state ----

int shell_debug_overlay_enabled(void) {
    return debug_overlay_enabled ? 1 : 0;
}

void shell_set_debug_overlay(int enabled) {
    debug_overlay_enabled = (uint8_t)(enabled ? 1 : 0);
}

int shell_ui_scale(void) {
    return get_ui_scale();
}

int shell_display_profile_count(void) {
    return display_profile_count();
}

int shell_display_profile_available(int idx) {
    return display_profile_available(idx);
}

const char* shell_display_profile_label(int idx) {
    if (idx < 0 || idx >= display_profile_count()) return "Unknown";
    return g_display_profiles[idx].label;
}

int shell_display_profile_current(void) {
    return setting_display_profile;
}

void shell_set_display_profile(int idx) {
    setting_display_profile = idx;
    apply_display_profile();
}

void kernel_main(multiboot_info_t* mb_info) {
    serial_init();
    serial_write_string("GamerOS Starting...\n");
    debug_log_message("Boot: kernel_main start");
    {
        char boot_line[64];
        sprintf(boot_line, "Boot protocol: %s\n", multiboot2_protocol_name());
        serial_write_string(boot_line);
        debug_log_message(boot_line);
    }
    
    // Prefer the multiboot framebuffer; fall back to VGA.
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
    
    keyboard_init();
    debug_log_message("Boot: keyboard init ok");
    mouse_init();
    debug_log_message("Boot: mouse init ok");
    usb_init();
    xhci_init();
    vmware_mouse_init();
    vboxguest_init();
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
      ensure_storage_initialized();
      debug_log_message("Boot: entering main loop");
    debug_present_boot_status();

    // VMware: polling-only input; IRQ paths can still fault while dragging.
    __asm__ volatile ("cli");

    // Main loop
    int32_t last_mx = -1, last_my = -1;
    uint8_t scene_dirty = 0;

    // Paint one frame first so a noisy PS/2 controller cannot trap the VM on black.
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

        // Keyboard first: both devices share the PS/2 output buffer.
        keyboard_poll();
        mouse_poll();
        // Non-PS/2 sources run after the PS/2 drains (they read position+buttons at once).
        usb_poll();
        xhci_poll();
        vmware_mouse_poll();
        vboxguest_mouse_poll();
        
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
        // Normalize wheel direction so wheel-up scrolls content up.
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

// ---- App management stubs -------------------------------------------------
//
// TODO: Implement real app launch: load the GOSAPP binary, map it into memory,
// and create its window instead of only listing descriptors.
static const app_descriptor_t g_app_descriptors[] = {
    {"NOTEPAD.EXE",  "C:/GamerOS/System32/NOTEPAD.EXE",  "Notepad",
     "C:/GamerOS/Apps/Manifests/Notepad.gosapp",  "C:/GamerOS/Apps/Notepad",
     "C:/Users/Admin/AppData/Notepad",  WIN_NOTEPAD,  40, 30},
    {"SETTINGS.EXE", "C:/GamerOS/System32/SETTINGS.EXE", "Settings",
     "C:/GamerOS/Apps/Manifests/Settings.gosapp", "C:/GamerOS/Apps/Settings",
     "C:/Users/Admin/AppData/Settings", WIN_SETTINGS, 8, 6},
    {"EXPLORER.EXE", "C:/GamerOS/System32/EXPLORER.EXE", "File Explorer",
     "C:/GamerOS/Apps/Manifests/Explorer.gosapp", "C:/GamerOS/Apps/Explorer",
     "C:/Users/Admin/AppData/Explorer", WIN_EXPLORER, 8, 6},
    {"ABOUT.EXE",    "C:/GamerOS/System32/ABOUT.EXE",    "About GamerOS",
     "C:/GamerOS/Apps/Manifests/About.gosapp",    "C:/GamerOS/Apps/About",
     "C:/Users/Admin/AppData/About",    WIN_ABOUT,   60, 40},
};
#define APP_DESCRIPTOR_COUNT (sizeof(g_app_descriptors) / sizeof(g_app_descriptors[0]))

int apps_get_count(void) {
    return (int)APP_DESCRIPTOR_COUNT;
}

const app_descriptor_t* apps_get_by_index(int idx) {
    if (idx < 0 || idx >= (int)APP_DESCRIPTOR_COUNT) return 0;
    return &g_app_descriptors[idx];
}

const app_descriptor_t* apps_find_by_exe(const char* exe_name) {
    if (!exe_name) return 0;
    for (int i = 0; i < (int)APP_DESCRIPTOR_COUNT; i++) {
        if (strcmp(g_app_descriptors[i].exe_name, exe_name) == 0) {
            return &g_app_descriptors[i];
        }
    }
    return 0;
}

int apps_resolve_launch(const char* exe_name, int* out_win_type, int* out_x, int* out_y) {
    const app_descriptor_t* desc = apps_find_by_exe(exe_name);
    if (!desc) return 0;
    if (out_win_type) *out_win_type = desc->window_type;
    if (out_x) *out_x = desc->launch_x;
    if (out_y) *out_y = desc->launch_y;
    return 1;
}
