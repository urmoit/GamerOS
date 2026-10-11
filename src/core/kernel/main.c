#include <graphics.h>
#include <mouse.h>
#include <keyboard.h>
#include <serial.h>
// ports.h also declares the port I/O helpers and the device port constants;
// including io.h as well would only duplicate those prototypes.
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
#include "../../resources/wallpapers/background_wallpaper.h"
#include <stdint.h>
#include <string.h>

// Window state kinds. These mirror the app_descriptor_t window_type values.
#define APP_WINDOW_NOTEPAD 1
#define APP_WINDOW_MYCOMP 2
#define APP_WINDOW_ABOUT 3
#define APP_WINDOW_SETTINGS 4
#define APP_WINDOW_EXPLORER 5

// Forward declarations for UI functions only
static const char* settings_ui_panel_title(void);
static const char* settings_ui_tab_name(int tab_idx);
static const char* settings_ui_changelog_md_path(void);
static const char* settings_ui_changelog_md_text(void);

// Window states
#define MAX_WINDOWS 6
#define WIN_NONE    0
#define WIN_NOTEPAD APP_WINDOW_NOTEPAD
#define WIN_MYCOMP  APP_WINDOW_MYCOMP
#define WIN_ABOUT   APP_WINDOW_ABOUT
#define WIN_SETTINGS APP_WINDOW_SETTINGS
#define WIN_EXPLORER APP_WINDOW_EXPLORER

// Desktop shell sizing (compact XP-like layout for 320x200)
// Compact UI text renders the canonical 8x8 font at its natural advance.
// Cropping glyphs to 6 of 8 columns (the old behaviour) sheared letters into
// vertical smears, so both the sample width and the advance are 8.
#define START_BTN_X            4
// Wide enough for the flag plus "Start" at the full 8px advance (5 * 8 = 40px).
#define START_BTN_W            64
#define START_BTN_H            16
#define TASKBTN_W              70
#define TASKBTN_H              16
// The rail has to fit "Admin" at the full 8px advance (40px) plus padding,
// otherwise the profile label bled into the content column.
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
static uint8_t setting_show_seconds = 0;
static uint8_t setting_desktop_glow = 0;
static uint8_t setting_compact_mode = 1;
static int setting_display_profile = 0;
static int settings_tab = 0;

// Toggles exposed on the non-System pages.
static uint8_t setting_bluetooth = 0;
static uint8_t setting_network = 0;
static uint8_t setting_high_contrast = 0;
static uint8_t setting_large_text = 0;

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
// The Settings changelog is parsed and drawn by src/ui/markdown/markdown.c.
static uint8_t settings_md_loaded = 0;
// started silently dropping the tail.

typedef enum {
    SETTINGS_UPDATE_VIEW_CHANGELOG = 0,
    SETTINGS_UPDATE_VIEW_ROADMAP = 1
} settings_update_view_t;

static int settings_update_view = SETTINGS_UPDATE_VIEW_CHANGELOG;

// Hit-box state published by draw_window() so handle_settings_click() tests the
// exact rectangles that were rendered, instead of duplicating layout constants.
#define SETTINGS_MAX_TOGGLES 8

typedef enum {
    SETTINGS_TOG_COMPACT = 0,
    SETTINGS_TOG_SECONDS,
    SETTINGS_TOG_DEBUG,
    SETTINGS_TOG_BLUETOOTH,
    SETTINGS_TOG_NETWORK,
    SETTINGS_TOG_CONTRAST,
    SETTINGS_TOG_LARGE_TEXT
} settings_toggle_id_t;

typedef struct {
    const char* label;
    int is_on;
    int id;
} settings_toggle_desc_t;

static int settings_tog_count = 0;
static int settings_tog_hit_x = 0;
static int settings_tog_hit_w = 0;
static int settings_tog_hit_y[SETTINGS_MAX_TOGGLES];
static int settings_tog_id[SETTINGS_MAX_TOGGLES];

static int settings_res_hit_x = 0;
static int settings_res_hit_y[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
static int settings_res_hit_count = 0;
static int settings_res_hit_h = 0;

// Sub-button hit boxes for the GamerOS Update view switch.
static int settings_view_hit_x = 0;
static int settings_view_hit_y = 0;
static int settings_view_hit_w = 0;
static int settings_view_hit_h = 0;
static int settings_view_hit_gap = 0;

// Rows the changelog viewer actually rendered last frame; the wheel handler
// scrolls by this rather than recomputing a viewport size of its own.

#define EXPLORER_MAX_ENTRIES 14
// "This PC" renders one block per volume in the style of a Windows drive list:
// name, usage bar, and "X.X GB free of YYY GB". These rows are taller than the
// single-line rows used for files and folders.
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

// Navigation history so Back/Forward are real rather than decorative.
#define EXPLORER_HISTORY 16
static char explorer_hist_path[EXPLORER_HISTORY][MAX_FILENAME_LEN];
static uint8_t explorer_hist_thispc[EXPLORER_HISTORY];
static int explorer_hist_count = 0;
static int explorer_hist_pos = -1;

// Hit boxes published by draw_window() for the Explorer chrome.
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
    return ((time_chars + 1 + date_chars) * FONT_CW) + 8;
}

// ---------------------------------------------------------------------------
// Dock geometry
//
// The dock is a floating rounded panel centred on the bottom edge, sized to its
// contents rather than to the screen. Both the renderer and the hit tester call
// dock_compute_layout, so the clickable areas cannot drift away from the pixels:
// when task buttons appear or vanish the dock re-centres and both sides agree.
// ---------------------------------------------------------------------------

#define DOCK_PAD_X       8    // Inner padding left and right of the items.
#define DOCK_PAD_Y       5    // Inner padding above and below the items.
#define DOCK_GAP_BOTTOM  7    // Gap between the dock and the bottom of the screen.
#define DOCK_GAP_ITEMS   6    // Gap between Start, the task run and the clock.
#define DOCK_RADIUS      8    // Corner radius of the panel.
#define DOCK_SHADOW      3    // Soft drop shadow depth, in whole pixels.

// Vertical space the dock claims from the bottom edge, including the gap beneath
// it. Window placement, the desktop context menu and the debug overlay all clamp
// against this so nothing ends up hidden underneath the dock. Expressed in
// terms of the same pieces dock_compute_layout uses, so the two cannot drift.
#define DOCK_INNER_H     ((TASKBTN_H > START_BTN_H) ? TASKBTN_H : START_BTN_H)
#define DOCK_RESERVED_H  (DOCK_INNER_H + (DOCK_PAD_Y * 2) + DOCK_GAP_BOTTOM)

typedef struct {
    int x, y, w, h;         // Panel rectangle.
    int start_x, start_y;   // Start button.
    int taskbtn_w, taskbtn_y;
    int task_x, task_count; // Left edge of the task run, and how many fit.
    int clock_x, clock_y, clock_w;
} dock_layout_t;

static void dock_compute_layout(dock_layout_t* d) {
    if (!d) return;

    int taskbtn_w = setting_compact_mode ? TASKBTN_W : (TASKBTN_W + 16);
    int clock_w = taskbar_clock_width();

    int inner_h = (TASKBTN_H > START_BTN_H) ? TASKBTN_H : START_BTN_H;
    int h = inner_h + (DOCK_PAD_Y * 2);

    // Start and the clock are always present, so they come off the top before
    // deciding how many task buttons are allowed to claim space.
    int fixed_w = (DOCK_PAD_X * 2) + START_BTN_W + clock_w;
    int max_w = (int)current_width - fixed_w - (DOCK_GAP_ITEMS * 2) - 8;
    if (max_w < 0) max_w = 0;
    int max_count = max_w / (taskbtn_w + 2);

    int task_count = 0;
    for (int i = 0; i < MAX_WINDOWS && task_count < max_count; i++) {
        if (windows[i].active) task_count++;
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
    // Layout shared by the renderer and the click handler. These used to be
    // duplicated in both places, which is how the rail and the item rows
    // drifted out of alignment.
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

// Record the current location. `truncate_forward` drops the redo entries that
// follow the current position, which is what navigating from a rewound history
// should do.
static void explorer_history_push(uint8_t truncate_forward) {
    if (explorer_hist_count > 0) {
        // Don't record a duplicate of where we already are.
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
        // Shift out the oldest entry.
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

// Tenths of a GiB, computed with shifts only. The kernel links without libgcc,
// so 64-bit division is unavailable here and must not be introduced.
static uint64_t bytes_to_gb_tenths(uint64_t bytes) {
    return (bytes * 10) >> 30;
}

static void append_unsigned(char* out, int out_size, int* pos, uint64_t v) {
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

static void append_text(char* out, int out_size, int* pos, const char* s) {
    if (!s) return;
    while (*s && *pos + 1 < out_size) out[(*pos)++] = *s++;
}

// "47.0 GB free of 474 GB", matching the Windows drive list. Volumes smaller
// than 1 GiB are reported in MB, otherwise a RAM-backed fallback would render
// as "0.0 GB free of 0 GB".
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

    // Report what the medium actually reported, so a boot log proves whether
    // capacity detection worked rather than just asserting it did.
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

// Details-view column widths. The Date modified column needs ~18 characters
// for "10/10/2026 11:59 PM" at the 8px advance, so these are generous and the
// Name column simply absorbs whatever is left.
#define EXPL_COL_DATE_W 152
#define EXPL_COL_TYPE_W 124
#define EXPL_COL_SIZE_W 86

// "10/10/2026 11:59 PM" - matches the Windows details view.
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

// Human-readable type label derived from the extension, the way Explorer shows
// it. Unrecognised extensions fall back to the generic "File".
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

// "0 bytes", "1.2 KB", "3.4 MB". Shifts only, no 64-bit division.
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

        // "<device label> (C:)" using whatever the medium reported, so a real
        // disk shows its model rather than a generic placeholder.
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

// Navigate to a concrete path, recording it in history.
static void explorer_navigate_to(const char* path) {
    if (!path) return;
    strncpy(explorer_path, path, sizeof(explorer_path) - 1);
    explorer_path[sizeof(explorer_path) - 1] = 0;
    explorer_this_pc_view = 0;
    explorer_history_push(0);
    explorer_selected = -1;
    explorer_refresh();
}

// Quick-access destinations shown in the sidebar.
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
        "shell=GamerOS\nbuild=1.560\nsystem=C:/GamerOS/System32\napps=C:/GamerOS/Apps\nregistry=C:/GamerOS/Registry\n");
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

// Compact UI text uses the canonical 8x8 bitmap font at its natural 8px advance.
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
    const char* build = "Build 1.560";

    int brand_w = compact_text_width(brand);
    int tag_w = compact_text_width(tag);
    int build_w = compact_text_width(build);

    const int accent_w = 4;
    const int pad_x = 8;
    const int gap = FONT_CW;

    // The build badge is right-aligned on the lower row, so the card has to be
    // as wide as (tag + gap + build) whenever that exceeds the brand line.
    int brand_row_w = brand_w;
    int meta_row_w = tag_w + gap + build_w;
    int inner_w = (brand_row_w > meta_row_w) ? brand_row_w : meta_row_w;

    int card_w = inner_w + pad_x * 2 + accent_w;
    int card_h = 26;
    int x = (int)current_width - card_w - 10;
    int y = (int)current_height - DOCK_RESERVED_H - card_h - 10;
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

    // The embedded wallpaper asset is essentially transparent/black. If it
    // samples as too dark, brighten the generated gradient so the desktop is
    // never painted near-black.
    int too_dark = wallpaper_is_too_dark();

    if (graphics_is_truecolor()) {
        uint32_t w = current_width;
        uint32_t h = (uint32_t)desktop_h;

        // Faster row-by-row gradient using fill_rect
        for (uint32_t y = 0; y < h; y++) {
            uint32_t t = (y * 255) / (h > 1 ? h - 1 : 1);
            uint32_t r = 58 - ((40 * t) / 255);
            uint32_t g = 110 - ((55 * t) / 255);
            uint32_t b = 165 - ((30 * t) / 255);
            uint32_t rgb = (r << 16) | (g << 8) | b;
            if (too_dark) {
                // Lift the gradient floor so the desktop stays readable.
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

        // Subtle lighter band across the middle
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

        // GamerOS brand watermark - subtle corner decoration
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

    // Indexed mode - smooth gradient
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
        // Accent stripe
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

    // The menu hangs off the dock's left edge so the two read as one control,
    // then gets nudged back on screen if the centred dock would push it off.
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
    if (tab_idx >= 0 && tab_idx < SETTINGS_TAB_COUNT) {
        settings_tab = tab_idx;
    }
}

static void settings_md_parse(const char* markdown) {
    md_parse(markdown);
    if (md_was_truncated()) {
        debug_log_message("Settings changelog: document truncated");
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

    // Title and subtitle hang off the panel's content column. The old code
    // centred the title on the *screen*, which pushed it out past the panel
    // edge on anything wider than the panel.
    int title_x = panel_inner_x;
    int title_y = panel_y + 12;

    // Spinner sits on its own row with the label beneath it; previously the
    // label was drawn across the ring at the same y and overwrote the dots.
    int loader_cx = panel_x + (panel_w / 2);
    int loader_cy = panel_y + 88;
    int loading_w = compact_text_width(loading_label);
    int loading_x = loader_cx - (loading_w / 2);
    int loading_y = loader_cy + 26;

    int bg_h = (int)current_height;
    int frames = 48;

    // Draw background once (static)
    if (graphics_is_truecolor()) {
        uint32_t w = current_width;
        uint32_t h = (uint32_t)bg_h;
        // Deep blue gradient
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

    // Pre-draw panel background (static part)
    fill_chamfer_rect(panel_x, panel_y, panel_w, panel_h, XP_WHITE);
    draw_chamfer_rect(panel_x, panel_y, panel_w, panel_h, XP_LGRAY);
    draw_chamfer_rect(panel_x + 1, panel_y + 1, panel_w - 2, panel_h - 2, XP_DGRAY);
    fill_chamfer_rect(panel_x + 4, panel_y + 4, rail_w, panel_h - 8, XP_LGRAY);
    fill_rect(panel_x + 5, panel_y + 5, rail_w - 2, 2, XP_WHITE);

    // Avatar chip, centred in the rail.
    int chip_w = 24;
    int chip_x = panel_x + 4 + ((rail_w - chip_w) / 2);
    fill_chamfer_rect(chip_x, panel_y + 12, chip_w, 14, XP_LBLUE);
    draw_compact_string(chip_x + ((chip_w - FONT_CW) / 2), panel_y + 15, "G", XP_BLACK);

    // Brand line inside the content column.
    fill_chamfer_rect(panel_inner_x, panel_y + 8, panel_inner_w, 18, XP_WHITE);
    fill_rect(panel_inner_x + 1, panel_y + 9, panel_inner_w - 2, 2, XP_LBLUE);
    draw_compact_string(title_x, title_y, title, XP_BLACK);

    fill_chamfer_rect(panel_inner_x, panel_y + 32, panel_inner_w, 14, XP_LGRAY);
    draw_chamfer_rect(panel_inner_x, panel_y + 32, panel_inner_w, 14, XP_DGRAY);
    draw_compact_string_clipped(panel_inner_x + 6, panel_y + 36,
                                (panel_inner_w - 10) / FONT_CW, subtitle, XP_BLACK);

    // Loading label sits below the ring, centred under it.
    draw_compact_string(loading_x, loading_y, loading_label, XP_DGRAY);

    // Pre-compute ring positions
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

        // Only redraw dots that changed
        if (lit > prev_lit) {
            for (int i = prev_lit; i < lit; i++) {
                int px = loader_cx + ring_dx[i];
                int py = loader_cy + ring_dy[i];
                fill_rect(px - 2, py - 2, 5, 5, XP_BLUE);
            }
            // Highlight current dot
            if (lit > 0 && lit <= 24) {
                int px = loader_cx + ring_dx[lit - 1];
                int py = loader_cy + ring_dy[lit - 1];
                fill_rect(px - 2, py - 2, 5, 5, XP_LBLUE);
            }
            // Unlit dots
            for (int i = lit; i < 24; i++) {
                int px = loader_cx + ring_dx[i];
                int py = loader_cy + ring_dy[i];
                fill_rect(px - 2, py - 2, 5, 5, XP_LGRAY);
            }
        }
        prev_lit = lit;

        swap_buffers();
        // Short delay - no busy loop
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

// Initialize windows
void init_windows(void) {
    for (int i = 0; i < MAX_WINDOWS; i++) {
        windows[i].active = 0;
    }
    notepad_app_init();
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
                strncpy(windows[i].title, settings_ui_panel_title(), 31);
                windows[i].w = (int)current_width - 16;
                windows[i].h = (int)current_height - DOCK_RESERVED_H - 22;
                windows[i].x = 8;
                windows[i].y = 6;
                for (int si = 0; si < SETTINGS_TAB_COUNT; si++) settings_scroll_top[si] = 0;
            } else if (type == WIN_EXPLORER) {
                strncpy(windows[i].title, "File Explorer", 31);
                windows[i].w = (int)current_width - 16;
                windows[i].h = (int)current_height - DOCK_RESERVED_H - 22;
                windows[i].x = 8;
                windows[i].y = 6;
                explorer_open_this_pc();
                // Start the history at "This PC" so Back is available immediately.
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

// Close window
void close_window(int idx) {
    if (idx >= 0 && idx < MAX_WINDOWS && windows[idx].active) {
        if (windows[idx].type == WIN_NOTEPAD && notepad_app_is_dirty() && shell_storage_ready()) {
            notepad_app_save();
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

// Draw a horizontal on/off switch.
//
// Renders a chamfered pill with a sliding knob: the track turns accent blue when
// enabled, and the knob moves from the left (off) to the right (on).
static void draw_toggle_switch(int x, int y, int is_on) {
    const int tw = 20;
    const int th = 9;
    if (x < 0 || y < 0) return;

    // Track.
    uint8_t track = is_on ? XP_LBLUE : XP_LGRAY;
    fill_chamfer_rect(x, y, tw, th, track);
    draw_chamfer_rect(x, y, tw, th, XP_DGRAY);
    // Highlight along the top edge for a slightly raised look.
    fill_rect(x + 1, y + 1, tw - 2, 1, is_on ? XP_LBLUE : XP_WHITE);

    // Knob slides across the track.
    const int knob_w = 7;
    const int knob_h = 5;
    const int inset = 2;
    int knob_x = is_on ? (x + tw - knob_w - inset) : (x + inset);
    int knob_y = y + (th - knob_h) / 2;
    fill_chamfer_rect(knob_x, knob_y, knob_w, knob_h, XP_WHITE);
    draw_chamfer_rect(knob_x, knob_y, knob_w, knob_h, XP_DGRAY);
}

// Draw one labelled settings toggle row: label on the left, state text and a
// right-aligned switch on the right. Returns the height consumed.
static int draw_settings_toggle(int content_x, int content_w, int row_y,
                                const char* label, int is_on, int max_chars) {
    const int toggle_w = 20;
    // Right-align the switch inside the content pane.
    int toggle_x = content_x + content_w - toggle_w - 6;
    if (toggle_x < content_x) toggle_x = content_x;

    draw_compact_string_clipped(content_x + 6, row_y + 1, max_chars, label, XP_BLACK);

    // State text sits just left of the switch.
    const char* state = is_on ? "On" : "Off";
    int state_x = toggle_x - 6 - ((int)strlen(state) * FONT_CW);
    if (state_x > content_x + 6) {
        draw_compact_string(state_x, row_y + 1, state, is_on ? XP_BLUE : XP_DGRAY);
    }

    draw_toggle_switch(toggle_x, row_y, is_on);
    return 12;
}

// Section heading: coloured label plus a short accent tick and a faint rule.
// Returns the Y at which the section body may start.
// ---- Time zone dropdown ------------------------------------------------------
// A combo box: one row showing the current selection with a drop arrow, which
// expands to a scrolling list on click. The list is longer than the pane, so the
// mouse wheel scrolls it. Row geometry is published for the click and wheel
// handlers rather than recomputed there, which is how the other Settings
// controls stay aligned.
#define TZ_LIST_VISIBLE 8
#define TZ_ROW_H       11
#define TZ_LIST_BTN    11
#define TZ_COMBO_H     13

static int settings_tz_scroll = 0;
static int settings_tz_open = 0;
static int settings_tz_list_x = 0;
static int settings_tz_list_w = 0;
static int settings_tz_list_y = 0;
static int settings_tz_list_bottom = 0;
static int settings_tz_combo_y = 0;
static int settings_tz_up_x = 0;
static int settings_tz_up_y = 0;
static int settings_tz_down_x = 0;
static int settings_tz_down_y = 0;
static int settings_tz_btn = TZ_LIST_BTN;

// Short label for a zone, with the "UTC+05:30  " prefix stripped for readability.
static const char* settings_tz_selected_label(int index) {
    const tz_info_t* z = tz_get(index);
    if (!z) return "Unknown";
    const char* n = z->name;
    while (*n == 'U' || *n == 'T' || *n == 'C' || *n == '+' || *n == '-' ||
           (*n >= '0' && *n <= '9') || *n == ':') n++;
    while (*n == ' ') n++;
    return n;
}

// Solid triangle used for the drop arrow and the scroll buttons. The 8x8 font has
// no such glyph, so it is drawn rather than printed.
static void draw_tz_chevron_down(int x, int y, uint8_t color) {
    for (int row = 0; row < 4; row++) {
        int w = row * 2 + 1;
        fill_rect(x - w / 2, y + row, w, 1, color);
    }
}

static void draw_tz_chevron_up(int x, int y, uint8_t color) {
    for (int row = 0; row < 4; row++) {
        int w = (3 - row) * 2 + 1;
        fill_rect(x - w / 2, y + row, w, 1, color);
    }
}

static void settings_tz_clamp_scroll(int selected) {
    int total = tz_count();
    int max_scroll = total - TZ_LIST_VISIBLE;
    if (max_scroll < 0) max_scroll = 0;
    if (settings_tz_scroll < 0) settings_tz_scroll = 0;
    if (settings_tz_scroll > max_scroll) settings_tz_scroll = max_scroll;
    if (settings_tz_scroll > selected - 1) settings_tz_scroll = selected - 1;
    if (settings_tz_scroll < selected - TZ_LIST_VISIBLE + 2) {
        settings_tz_scroll = selected - TZ_LIST_VISIBLE + 2;
    }
    if (settings_tz_scroll < 0) settings_tz_scroll = 0;
    if (settings_tz_scroll > max_scroll) settings_tz_scroll = max_scroll;
}

static void settings_tz_scroll_by(int delta) {
    int total = tz_count();
    int max_scroll = total - TZ_LIST_VISIBLE;
    if (max_scroll < 0) max_scroll = 0;
    settings_tz_scroll += delta;
    if (settings_tz_scroll < 0) settings_tz_scroll = 0;
    if (settings_tz_scroll > max_scroll) settings_tz_scroll = max_scroll;
}

// Draws the combo row, plus the list when open. Returns the height consumed.
static int draw_settings_timezone_list(int cx, int cw, int y) {
    int selected = tz_current_index();
    settings_tz_clamp_scroll(selected);

    int total = tz_count();
    settings_tz_list_x = cx;
    settings_tz_list_w = cw;
    settings_tz_btn = TZ_LIST_BTN;
    settings_tz_combo_y = y;
    settings_tz_list_y = y + TZ_COMBO_H + 2;
    settings_tz_list_bottom = settings_tz_list_y;

    // ---- Combo row ---------------------------------------------------------
    fill_chamfer_rect(cx, y, cw, TZ_COMBO_H, settings_tz_open ? XP_LBLUE : XP_WHITE);
    draw_chamfer_rect(cx, y, cw, TZ_COMBO_H, XP_DGRAY);
    fill_rect(cx + 1, y + 1, cw - 2, 2, XP_LBLUE);

    char selected_buf[72];
    selected_buf[0] = 0;
    {
        int lp = 0;
        append_text(selected_buf, (int)sizeof(selected_buf), &lp,
                    settings_tz_selected_label(selected));
        append_text(selected_buf, (int)sizeof(selected_buf), &lp, "   (");
        append_unsigned(selected_buf, (int)sizeof(selected_buf), &lp, total);
        append_text(selected_buf, (int)sizeof(selected_buf), &lp, " zones)");
    }
    draw_compact_string_clipped(cx + 6, y + 3, (cw - 26) / FONT_CW, selected_buf, XP_BLACK);
    draw_tz_chevron_down(cx + cw - 8, y + 4, XP_DGRAY);

    if (!settings_tz_open) return TZ_COMBO_H + 2;

    // ---- Expanded list -----------------------------------------------------
    settings_tz_up_x = cx + cw - settings_tz_btn - 4;
    settings_tz_up_y = settings_tz_list_y;
    settings_tz_down_x = settings_tz_up_x;
    settings_tz_down_y = settings_tz_list_y + settings_tz_btn + 2;

    int list_y = settings_tz_list_y + settings_tz_btn + 4;
    int list_h = TZ_LIST_VISIBLE * TZ_ROW_H + 4;
    int frame_h = list_h + settings_tz_btn + 6;

    fill_chamfer_rect(cx, settings_tz_list_y, cw, frame_h, XP_WHITE);
    draw_chamfer_rect(cx, settings_tz_list_y, cw, frame_h, XP_DGRAY);

    // Scroll buttons, only when there is somewhere to scroll.
    if (total > TZ_LIST_VISIBLE) {
        int bx = settings_tz_up_x + settings_tz_btn / 2;
        fill_chamfer_rect(settings_tz_up_x, settings_tz_up_y, settings_tz_btn, settings_tz_btn, XP_LGRAY);
        draw_chamfer_rect(settings_tz_up_x, settings_tz_up_y, settings_tz_btn, settings_tz_btn, XP_DGRAY);
        draw_tz_chevron_up(bx, settings_tz_up_y + 3, XP_BLACK);

        fill_chamfer_rect(settings_tz_down_x, settings_tz_down_y, settings_tz_btn, settings_tz_btn, XP_LGRAY);
        draw_chamfer_rect(settings_tz_down_x, settings_tz_down_y, settings_tz_btn, settings_tz_btn, XP_DGRAY);
        draw_tz_chevron_down(bx, settings_tz_down_y + 4, XP_BLACK);
    }

    fill_chamfer_rect(cx + 3, list_y, cw - 6, list_h, XP_WHITE);
    draw_chamfer_rect(cx + 3, list_y, cw - 6, list_h, XP_DGRAY);

    for (int i = 0; i < TZ_LIST_VISIBLE; i++) {
        int idx = settings_tz_scroll + i;
        if (idx >= total) break;
        int ry = list_y + 2 + i * TZ_ROW_H;
        if (idx == selected) {
            fill_rect(cx + 4, ry, cw - 8, TZ_ROW_H - 1, XP_LBLUE);
        }
        draw_compact_string_clipped(cx + 7, ry + 1, (cw - 16) / FONT_CW,
                                    settings_tz_selected_label(idx), XP_BLACK);
    }

    settings_tz_list_bottom = settings_tz_list_y + frame_h;
    return (settings_tz_list_bottom - y) + 4;
}

// True when the cursor is over the combo row, or over the list while open.
static int settings_tz_hit(int mx, int my) {
    if (settings_tz_list_w <= 0) return 0;
    if (mx < settings_tz_list_x || mx >= settings_tz_list_x + settings_tz_list_w) return 0;
    if (my >= settings_tz_combo_y && my < settings_tz_combo_y + TZ_COMBO_H) return 1;
    if (settings_tz_open && my >= settings_tz_list_y && my < settings_tz_list_bottom) return 1;
    return 0;
}

void settings_tz_close(void) {
    settings_tz_open = 0;
}

// Wheel scrolling over the open list.
static int handle_settings_tz_wheel(int mx, int my, int8_t wheel_delta) {
    if (!settings_tz_open || wheel_delta == 0) return 0;
    if (!settings_tz_hit(mx, my)) return 0;
    settings_tz_scroll_by(-(int)wheel_delta);
    return 1;
}

// Click handling, published by the renderer above.
static int handle_settings_tz_click(int mx, int my) {
    if (settings_tz_list_w <= 0) return 0;
    int total = tz_count();
    int selected = tz_current_index();

    int in_combo = (mx >= settings_tz_list_x &&
                    mx < settings_tz_list_x + settings_tz_list_w &&
                    my >= settings_tz_combo_y &&
                    my < settings_tz_combo_y + TZ_COMBO_H);
    if (in_combo) {
        settings_tz_open = (uint8_t)!settings_tz_open;
        settings_tz_clamp_scroll(tz_current_index());
        return 1;
    }

    if (!settings_tz_open) return 0;
    if (mx < settings_tz_list_x || mx >= settings_tz_list_x + settings_tz_list_w) return 0;
    if (my < settings_tz_list_y || my >= settings_tz_list_bottom) return 0;

    if (total > TZ_LIST_VISIBLE) {
        if (mx >= settings_tz_up_x && mx < settings_tz_up_x + settings_tz_btn &&
            my >= settings_tz_up_y && my < settings_tz_up_y + settings_tz_btn) {
            settings_tz_scroll_by(-1);
            return 1;
        }
        if (mx >= settings_tz_down_x && mx < settings_tz_down_x + settings_tz_btn &&
            my >= settings_tz_down_y && my < settings_tz_down_y + settings_tz_btn) {
            settings_tz_scroll_by(1);
            return 1;
        }
    }

    int list_y = settings_tz_list_y + settings_tz_btn + 6;
    if (my < list_y) return 1;
    int rel = (my - list_y) / TZ_ROW_H;
    int idx = settings_tz_scroll + rel;
    if (idx < 0 || idx >= total) return 1;

    if (idx != selected) tz_set_current_index(idx);
    settings_tz_open = 0;   // picking an entry closes the list
    return 1;
}


static int draw_settings_section(int content_x, int content_w, int y, const char* title) {
    if (y < 0) return y;
    draw_compact_string_clipped(content_x + 6, y, (content_w - 12) / FONT_CW, title, XP_BLUE);
    int rule_y = y + 9;
    fill_rect(content_x + 6, rule_y, 12, 1, XP_LBLUE);
    fill_rect(content_x + 20, rule_y, content_w - 26, 1, XP_LGRAY);
    return rule_y + 4;
}

// Quieter sub-heading for groups inside a section.
static int draw_settings_subheader(int content_x, int content_w, int y, const char* title) {
    if (y < 0) return y;
    draw_compact_string_clipped(content_x + 6, y, (content_w - 12) / FONT_CW, title, XP_DGRAY);
    return y + 11;
}

// Label on the left, value right-aligned. Returns the height consumed.
static int draw_settings_kv(int content_x, int content_w, int y,
                            const char* key, const char* value) {
    if (y < 0 || !key || !value) return 11;
    draw_compact_string_clipped(content_x + 6, y, (content_w - 12) / FONT_CW, key, XP_DGRAY);

    int vw = (int)strlen(value) * FONT_CW;
    int vx = content_x + content_w - 6 - vw;
    int kx = content_x + 6;
    if (vx > kx + 54) {
        draw_compact_string(vx, y, value, XP_BLACK);
    } else {
        // Not enough room to right-align; place after the key instead.
        draw_compact_string_clipped(content_x + 50, y, (content_w - 56) / FONT_CW, value, XP_BLACK);
    }
    return 11;
}

// Backing card used to group related rows.
static void draw_settings_card(int content_x, int content_w, int y, int height) {
    if (height <= 0) return;
    int cx = content_x + 4;
    int cw = content_w - 12;
    if (cw <= 4) return;
    fill_chamfer_rect(cx, y, cw, height, XP_LGRAY);
    draw_chamfer_rect(cx, y, cw, height, XP_DGRAY);
    // Accent edge down the left side ties the group together.
    fill_rect(cx + 1, y + 1, 2, height - 2, XP_LBLUE);
}

// Publish a toggle's clickable rectangle for handle_settings_click().
static void settings_register_toggle(int x, int w, int y, int id) {
    if (settings_tog_count >= SETTINGS_MAX_TOGGLES) return;
    settings_tog_hit_x = x;
    settings_tog_hit_w = w;
    settings_tog_hit_y[settings_tog_count] = y;
    settings_tog_id[settings_tog_count] = id;
    settings_tog_count++;
}

// Render a group of toggles inside a card, registering each hit box.
// Returns the vertical space consumed.
static int draw_settings_toggle_group(int content_x, int content_w, int y,
                                      const settings_toggle_desc_t* items, int count) {
    const int ROW_H = 12;
    if (count <= 0) return 0;
    if (count > SETTINGS_MAX_TOGGLES) count = SETTINGS_MAX_TOGGLES;

    int card_h = 6 + count * ROW_H;
    draw_settings_card(content_x, content_w, y, card_h);

    int tx = content_x + 10;
    int tw = content_w - 22;
    int label_chars = (content_w - 60) / FONT_CW;
    if (label_chars < 4) label_chars = 4;

    for (int i = 0; i < count; i++) {
        int ry = y + 3 + i * ROW_H;
        settings_register_toggle(content_x, content_w - 16, ry, items[i].id);
        draw_settings_toggle(tx, tw, ry, items[i].label, items[i].is_on, label_chars);
    }
    return card_h + 8;
}

// Apply a toggle by id.
static void settings_apply_toggle(int id) {
    switch (id) {
        case SETTINGS_TOG_COMPACT:    setting_compact_mode = (uint8_t)!setting_compact_mode; break;
        case SETTINGS_TOG_SECONDS:    setting_show_seconds = (uint8_t)!setting_show_seconds; break;
        case SETTINGS_TOG_DEBUG:      debug_overlay_enabled = (uint8_t)!debug_overlay_enabled; break;
        case SETTINGS_TOG_BLUETOOTH:  setting_bluetooth = (uint8_t)!setting_bluetooth; break;
        case SETTINGS_TOG_NETWORK:    setting_network = (uint8_t)!setting_network; break;
        case SETTINGS_TOG_CONTRAST:   setting_high_contrast = (uint8_t)!setting_high_contrast; break;
        case SETTINGS_TOG_LARGE_TEXT: setting_large_text = (uint8_t)!setting_large_text; break;
        default: break;
    }
}

// Draw a small toolbar button with an optional enabled/disabled state.
void draw_tool_button(int x, int y, int w, int h, const char* label, int enabled) {
    uint8_t bg = enabled ? XP_WHITE : XP_LGRAY;
    uint8_t txt = enabled ? XP_BLACK : XP_DGRAY;
    fill_chamfer_rect(x, y, w, h, bg);
    draw_chamfer_rect(x, y, w, h, XP_DGRAY);
    if (enabled) fill_rect(x + 1, y + 1, w - 2, 1, XP_WHITE);
    int chars = (w - 6) / FONT_CW;
    draw_compact_string_clipped(x + 3, y + (h - 8) / 2, chars, label, txt);
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
        // Notepad renders itself; the shell only supplies the client rect.
        notepad_app_draw_content(x + 2, y + 2, w - 4, h - 4, is_active_window);
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
        draw_compact_string_clipped(content_x + 8, content_y + 7, (content_w - 16) / FONT_CW, "Modern Control Center", XP_BLACK);
        int content_chars = (content_w - 12) / FONT_CW;
        if (content_chars < 1) content_chars = 1;

        int nav_bottom = nav_y + nav_h;
        for (int i = 0; i < SETTINGS_TAB_COUNT; i++) {
            int ty = nav_y + 28 + i * 20;
            if (ty + 16 > nav_bottom) break;  // Window too short: stop drawing rows.
            fill_chamfer_rect(nav_x + 5, ty - 1, nav_w - 10, 16, (settings_tab == i) ? XP_LBLUE : XP_WHITE);
            draw_chamfer_rect(nav_x + 5, ty - 1, nav_w - 10, 16, XP_DGRAY);
            if (settings_tab == i) {
                fill_rect(nav_x + 7, ty + 1, 3, 12, XP_WHITE);
            }
            draw_compact_string_clipped(nav_x + 14, ty + 3, (nav_w - 24) / FONT_CW, settings_ui_tab_name(i), XP_BLACK);
        }

        // Reset per-frame hit-box registries. Only the active tab renders, so
        // whatever registers below is exactly what is clickable this frame.
        settings_tog_count = 0;
        settings_res_hit_count = 0;

        if (settings_tab == SETTINGS_TAB_SYSTEM) {
            // ---- System toggles -------------------------------------------
            settings_toggle_desc_t sys_toggles[] = {
                { "Taskbar compact", setting_compact_mode, SETTINGS_TOG_COMPACT },
                { "Clock seconds",   setting_show_seconds, SETTINGS_TOG_SECONDS },
                { "Debug window",    debug_overlay_enabled, SETTINGS_TOG_DEBUG }
            };

            int ry = draw_settings_section(content_x, content_w, body_y, "System");
            ry += draw_settings_toggle_group(content_x, content_w, ry, sys_toggles, 3);

            // ---- Display info ---------------------------------------------
            {
                char cur[40];
                char nat[40];
                char be[48];
                sprintf(cur, "%ux%u", (unsigned int)current_width, (unsigned int)current_height);
                sprintf(nat, "%ux%u", (unsigned int)graphics_get_native_width(), (unsigned int)graphics_get_native_height());
                sprintf(be, "%s", graphics_is_truecolor() ? "Framebuffer" : "VGA Safe");

                ry = draw_settings_section(content_x, content_w, ry, "Display");
                ry += draw_settings_kv(content_x, content_w, ry, "Resolution", cur);
                ry += draw_settings_kv(content_x, content_w, ry, "Native", nat);
                ry += draw_settings_kv(content_x, content_w, ry, "Backend", be);
                ry += 6;
            }

            // ---- Resolution list ------------------------------------------
            ry = draw_settings_subheader(content_x, content_w, ry, "Resolution");
            {
                int rx = content_x + 6;
                int rw = content_w - 16;
                settings_res_hit_x = rx;
                settings_res_hit_h = 12;
                for (int i = 0; i < display_profile_count(); i++) {
                    if (ry + 12 > content_y + content_h) break;
                    if (i >= 8) break;
                    int selected = (setting_display_profile == i);
                    int usable = display_profile_available(i);
                    uint8_t bg = selected ? XP_LBLUE : XP_WHITE;
                    fill_chamfer_rect(rx, ry, rw, 12, bg);
                    draw_chamfer_rect(rx, ry, rw, 12, XP_DGRAY);
                    // Selection marker.
                    if (selected) fill_rect(rx + 2, ry + 2, 2, 8, XP_WHITE);
                    draw_compact_string_clipped(rx + 7, ry + 2, (rw - 12) / FONT_CW,
                                                g_display_profiles[i].label,
                                                usable ? XP_BLACK : XP_DGRAY);
                    settings_res_hit_y[settings_res_hit_count++] = ry;
                    ry += 13;
                }
                ry += 6;
            }

            // ---- Storage ---------------------------------------------------
            ry = draw_settings_section(content_x, content_w, ry, "Storage");
            {
                int sc = fs_storage_get_device_count();
                char cnt[8];
                cnt[0] = (char)('0' + (sc > 9 ? 9 : sc));
                cnt[1] = 0;
                ry += draw_settings_kv(content_x, content_w, ry, "Devices", cnt);
                ry += draw_settings_kv(content_x, content_w, ry, "Volume", "System Disk");
            }
        } else if (settings_tab == SETTINGS_TAB_PERSONALIZATION) {
            char res_line[40];
            char scale_line[40];
            sprintf(res_line, "%ux%u", (unsigned int)current_width, (unsigned int)current_height);
            sprintf(scale_line, "x%d", get_ui_scale());

            int ry = draw_settings_section(content_x, content_w, body_y, "Theme");
            ry += draw_settings_kv(content_x, content_w, ry, "Theme", "GamerOS Modern");
            ry += draw_settings_kv(content_x, content_w, ry, "Color profile", "Blue");
            ry += 10;

            ry = draw_settings_section(content_x, content_w, ry, "Layout");
            ry += draw_settings_kv(content_x, content_w, ry, "Desktop", res_line);
            ry += draw_settings_kv(content_x, content_w, ry, "UI scale", scale_line);
            ry += draw_settings_kv(content_x, content_w, ry, "Taskbar", setting_compact_mode ? "Compact" : "Full");
        } else if (settings_tab == SETTINGS_TAB_BLUETOOTH_DEVICES) {
            settings_toggle_desc_t dev_toggles[] = {
                { "Bluetooth", setting_bluetooth, SETTINGS_TOG_BLUETOOTH }
            };

            int ry = draw_settings_section(content_x, content_w, body_y, "Wireless");
            ry += draw_settings_toggle_group(content_x, content_w, ry, dev_toggles, 1);
            ry = draw_settings_section(content_x, content_w, ry, "Input");
            ry += draw_settings_kv(content_x, content_w, ry, "Mouse", mouse_is_present() ? "PS/2 Compatible" : "Not detected");
            ry += draw_settings_kv(content_x, content_w, ry, "Keyboard", "PS/2 Standard");
            ry += 10;

            ry = draw_settings_section(content_x, content_w, ry, "Display");
            ry += draw_settings_kv(content_x, content_w, ry, "Output", "VGA / Framebuffer");
        } else if (settings_tab == SETTINGS_TAB_NETWORK_INTERNET) {
            settings_toggle_desc_t net_toggles[] = {
                { "Wi-Fi", setting_network, SETTINGS_TOG_NETWORK }
            };

            int ry = draw_settings_section(content_x, content_w, body_y, "Connection");
            ry += draw_settings_toggle_group(content_x, content_w, ry, net_toggles, 1);
            ry = draw_settings_section(content_x, content_w, ry, "Status");
            ry += draw_settings_kv(content_x, content_w, ry, "Network stack", setting_network ? "Enabled" : "Not enabled");
            ry += draw_settings_kv(content_x, content_w, ry, "Adapter", setting_network ? "Connecting" : "Virtual profile");
            ry += draw_settings_kv(content_x, content_w, ry, "IP address", setting_network ? "Pending" : "None");
        } else if (settings_tab == SETTINGS_TAB_APPS) {
            int ry = draw_settings_section(content_x, content_w, body_y, "Installed");
            {
                static const char* app_names[] = {
                    "Notepad", "Settings", "File Explorer", "About GamerOS"
                };
                int count = (int)(sizeof(app_names) / sizeof(app_names[0]));
                int card_h = 6 + count * 12;
                draw_settings_card(content_x, content_w, ry, card_h);
                for (int i = 0; i < count; i++) {
                    int ary = ry + 3 + i * 12;
                    // Bullet marker, then the display name.
                    fill_rect(content_x + 9, ary + 4, 2, 2, XP_LBLUE);
                    draw_compact_string_clipped(content_x + 15, ary + 2,
                                                (content_w - 60) / FONT_CW, app_names[i], XP_BLACK);
                }
                ry += card_h + 10;
            }
            ry = draw_settings_section(content_x, content_w, ry, "Paths");
            ry += draw_settings_kv(content_x, content_w, ry, "App root", "C:/GamerOS/Apps");
            ry += draw_settings_kv(content_x, content_w, ry, "Manifests", "C:/GamerOS/Apps/Manifests");
        } else if (settings_tab == SETTINGS_TAB_ACCOUNTS) {
            int ry = draw_settings_section(content_x, content_w, body_y, "Account");
            ry += draw_settings_kv(content_x, content_w, ry, "User", "Admin");
            ry += draw_settings_kv(content_x, content_w, ry, "Profile", "C:/Users/Admin");
            ry += 10;

            ry = draw_settings_section(content_x, content_w, ry, "Session");
            ry += draw_settings_kv(content_x, content_w, ry, "Mode", "Local");
            ry += draw_settings_kv(content_x, content_w, ry, "Security level", "System");
            ry += draw_settings_kv(content_x, content_w, ry, "Isolation", "Active");
        } else if (settings_tab == SETTINGS_TAB_TIME_LANGUAGE) {
            // Time section first, then language. Local time comes from the
            // selected time zone rather than straight off the RTC.
            uint32_t now_utc = get_unix_time();
            int tz_idx = tz_current_index();
            int tz_dst = (tz_offset_minutes_at(tz_idx, now_utc) !=
                          (tz_get(tz_idx) ? tz_get(tz_idx)->std_offset_min : 0));

            uint16_t ly = 0; uint8_t lmo = 1, ld = 1, lh = 0, lmi = 0, ls = 0;
            tz_local_time(now_utc, &ly, &lmo, &ld, &lh, &lmi, &ls);

            char tm[24];
            sprintf(tm, "%02u:%02u:%02u", (unsigned int)lh, (unsigned int)lmi, (unsigned int)ls);
            char tzline[24];
            tz_format_offset(tzline, (int)sizeof(tzline), tz_idx, now_utc);

            int ry = draw_settings_section(content_x, content_w, body_y, "Time");
            ry += draw_settings_kv(content_x, content_w, ry, "Local time", tm);
            ry += draw_settings_kv(content_x, content_w, ry, "Time zone",
                                   settings_tz_selected_label(tz_idx));
            ry += draw_settings_kv(content_x, content_w, ry, "UTC offset", tzline);
            ry += draw_settings_kv(content_x, content_w, ry, "Daylight saving",
                                   tz_dst ? "In effect" : "Inactive");
            // The dropdown: combo row, expanding to a scrolling list.
            // Advance from the published bottom edge rather than the helper's
            // return value, so the sections below can never sit underneath the
            // expanded list.
            {
                int tz_top = ry;
                draw_settings_timezone_list(content_x, content_w, ry);
                int tz_floor = settings_tz_list_bottom;
                if (tz_floor <= tz_top) tz_floor = tz_top + TZ_COMBO_H + 2;
                ry = tz_floor + 10;
            }

            ry = draw_settings_section(content_x, content_w, ry, "Language");
            ry += draw_settings_kv(content_x, content_w, ry, "Display", "English (US)");
            ry += draw_settings_kv(content_x, content_w, ry, "Region", "United States");
        } else if (settings_tab == SETTINGS_TAB_GAMING) {
            char gfx_line[48];
            char res_line[40];
            uint8_t bpp = graphics_get_bpp();
            sprintf(gfx_line, "%s %u-bpp",
                    graphics_is_truecolor() ? "True color" : "VGA indexed",
                    (unsigned int)bpp);
            sprintf(res_line, "%ux%u", (unsigned int)current_width, (unsigned int)current_height);

            int ry = draw_settings_section(content_x, content_w, body_y, "Graphics");
            ry += draw_settings_kv(content_x, content_w, ry, "Mode", gfx_line);
            ry += draw_settings_kv(content_x, content_w, ry, "Resolution", res_line);
            ry += draw_settings_kv(content_x, content_w, ry, "Backend",
                                   graphics_is_truecolor() ? "Framebuffer" : "VGA Safe");
            ry += 10;

            ry = draw_settings_section(content_x, content_w, ry, "Input");
            ry += draw_settings_kv(content_x, content_w, ry, "Latency mode", "Balanced");
            ry += draw_settings_kv(content_x, content_w, ry, "Polling", "Bounded (64 max)");
        } else if (settings_tab == SETTINGS_TAB_ACCESSIBILITY) {
            settings_toggle_desc_t a11y_toggles[] = {
                { "High contrast", setting_high_contrast, SETTINGS_TOG_CONTRAST },
                { "Large text",     setting_large_text,    SETTINGS_TOG_LARGE_TEXT }
            };

            int ry = draw_settings_section(content_x, content_w, body_y, "Display");
            ry += draw_settings_toggle_group(content_x, content_w, ry, a11y_toggles, 2);
            ry = draw_settings_section(content_x, content_w, ry, "Text");
            ry += draw_settings_kv(content_x, content_w, ry, "Font size", setting_large_text ? "Large" : "Standard");
        } else if (settings_tab == SETTINGS_TAB_PRIVACY_SECURITY) {
            int ry = draw_settings_section(content_x, content_w, body_y, "Security");
            ry += draw_settings_kv(content_x, content_w, ry, "Auth model", "Enabled");
            ry += draw_settings_kv(content_x, content_w, ry, "Isolation table", "Active");
            ry += draw_settings_kv(content_x, content_w, ry, "Policy", "Standard");
            ry += 10;

            ry = draw_settings_section(content_x, content_w, ry, "Data");
            ry += draw_settings_kv(content_x, content_w, ry, "User profile", "C:/Users/Admin");
            ry += draw_settings_kv(content_x, content_w, ry, "App data", "C:/Users/Admin/AppData");
            ry += draw_settings_kv(content_x, content_w, ry, "Telemetry", "None");
        } else if (settings_tab == SETTINGS_TAB_GAMEROS_UPDATE) {
            serial_write_string("[render tab10]");
            settings_md_ensure_loaded();
            serial_write_string("[md loaded]");
            int has_roadmap = (md_roadmap_start() >= 0);

            // Segmented control switching between the changelog and roadmap.
            int sub_x = content_x + 6;
            int sub_y = content_y + 5;
            int sub_gap = 6;
            int sub_w = (content_w - 12 - sub_gap) / 2;
            int sub_h = 15;
            if (sub_w < 56) sub_w = 56;
            settings_view_hit_x = sub_x;
            settings_view_hit_y = sub_y;
            settings_view_hit_w = sub_w;
            settings_view_hit_h = sub_h;
            settings_view_hit_gap = sub_gap;

            {
                const char* seg_labels[2] = { "Changelog", "Roadmap" };
                for (int i = 0; i < 2; i++) {
                    int bx = sub_x + i * (sub_w + sub_gap);
                    int active = (settings_update_view == i);
                    fill_chamfer_rect(bx, sub_y, sub_w, sub_h, active ? XP_LBLUE : XP_LGRAY);
                    draw_chamfer_rect(bx, sub_y, sub_w, sub_h, XP_DGRAY);
                    if (active) fill_rect(bx + 1, sub_y + 1, sub_w - 2, 2, XP_WHITE);
                    int chars = (sub_w - 10) / FONT_CW;
                    draw_compact_string_clipped(bx + 5, sub_y + 4, chars, seg_labels[i], XP_BLACK);
                }
            }

            // Markdown viewer body. The module owns wrapping, block styling and
            // the scroll clamp, so the shell only supplies the box.
            int view_y = sub_y + sub_h + 5;
            int view_h = content_y + content_h - view_y - 12;
            if (view_h < 20) view_h = 20;
            draw_settings_card(content_x, content_w, view_y, view_h);

            int text_top = view_y + 4;
            int body_h = view_h - 8;
            if (body_h < 20) body_h = 20;

            int scroll = settings_scroll_top[SETTINGS_TAB_GAMEROS_UPDATE];
            serial_write_string("[md draw]");
            md_draw(content_x, text_top, content_w - 8, body_h,
                    settings_update_view, &scroll);
            serial_write_string("[md drawn]");
            settings_scroll_top[SETTINGS_TAB_GAMEROS_UPDATE] = scroll;

            if (!has_roadmap && settings_update_view == MD_VIEW_ROADMAP) {
                // No roadmap section in this changelog. Say so rather than
                // duplicating the changelog, which made the toggle look broken.
                draw_compact_string_clipped(content_x + 12, text_top + 2,
                                            (content_w - 24) / FONT_CW,
                                            "No roadmap in this changelog.", XP_DGRAY);
            }

            // Footer hint, pinned to the bottom of the pane.
            draw_compact_string_clipped(content_x + 8, content_y + content_h - 10,
                                        content_chars, "Scroll with mouse wheel", XP_DGRAY);
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
        draw_compact_string_clipped(content_x, content_y + 12, content_chars, "Build: 1.560", XP_BLACK);
        draw_compact_string_clipped(content_x, content_y + 24, content_chars, "Design language: GamerOS Modern", XP_BLACK);
        draw_compact_string_clipped(content_x, content_y + 42, content_chars, "System", XP_DGRAY);
        draw_compact_string_clipped(content_x, content_y + 54, content_chars, "Kernel: x86_64", XP_BLACK);
        draw_compact_string_clipped(content_x, content_y + 66, content_chars, gfx_line, XP_BLACK);
        draw_compact_string_clipped(content_x, content_y + 78, content_chars, "System32: C:/GamerOS/System32", XP_BLACK);
        draw_compact_string_clipped(content_x, content_y + 96, content_chars, "Apps", XP_DGRAY);
        draw_compact_string_clipped(content_x, content_y + 108, content_chars, "Apps: *.EXE (NOTEPAD/SETTINGS/EXPLORER/ABOUT)", XP_BLACK);
        draw_compact_string_clipped(content_x, content_y + 120, content_chars, "Author: Chosentechies", XP_BLACK);
    } else if (win->type == WIN_EXPLORER) {
        // ---- Layout ----------------------------------------------------------
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

        // ---- Client background ----------------------------------------------
        fill_chamfer_rect(x + 4, cy, w - 8, cb - cy, XP_LGRAY);
        draw_chamfer_rect(x + 4, cy, w - 8, cb - cy, XP_DGRAY);

        // ---- Toolbar ----------------------------------------------------------
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

        // ---- Breadcrumb ------------------------------------------------------
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

        // ---- Sidebar ---------------------------------------------------------
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

        // ---- Entry list ------------------------------------------------------
        int list_x = sb_x + SIDEBAR_W + 5;
        int list_w = (x + w - 6) - list_x;
        if (list_w < 20) list_w = 20;
        fill_chamfer_rect(list_x, body_y, list_w, body_h, XP_WHITE);
        draw_chamfer_rect(list_x, body_y, list_w, body_h, XP_DGRAY);

        // ---- Details view columns: Name | Date modified | Type | Size ----------
        // Columns are laid out from the right edge inward so Name always absorbs
        // the remainder, and the whole group collapses to Name + Size when the
        // window is too narrow to hold the date and type columns legibly.
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

                // Drive badge.
                fill_rect(list_x + 6, ry + 3, 7, 6, XP_DGRAY);
                fill_rect(list_x + 6, ry + 3, 7, 2, XP_WHITE);

                draw_compact_string_clipped(list_x + 18, ry + 2, name_chars,
                                            explorer_drive_labels[i], XP_BLACK);

                // Usage bar: filled portion = used, remainder = free.
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

        // ---- Status bar ------------------------------------------------------
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

    // Modern light panel style (wider, cleaner spacing, larger action rows).
    fill_chamfer_rect(menu_x, menu_y, menu_w, menu_h, XP_WHITE);
    draw_chamfer_rect(menu_x, menu_y, menu_w, menu_h, XP_LGRAY);
    draw_chamfer_rect(menu_x + 1, menu_y + 1, menu_w - 2, menu_h - 2, XP_DGRAY);

    // Left profile rail + avatar chip. The chip and both labels are centred on
    // the rail's own centre line rather than at fixed offsets.
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

    // Header + search strip.
    fill_chamfer_rect(panel_x, menu_y + 4, panel_w, 20, XP_WHITE);
    fill_rect(panel_x + 1, menu_y + 5, panel_w - 2, 2, XP_LBLUE);
    draw_compact_string(panel_x + 6, menu_y + 10, "GamerOS", XP_BLACK);
    // Place "Launchpad" after the brand with a real gap instead of a magic x.
    int header_chars = (panel_w - 12) / FONT_CW;
    draw_compact_string_clipped(panel_x + 6 + (compact_text_width("GamerOS") + FONT_CW),
                                menu_y + 11, header_chars, "Launchpad", XP_DGRAY);

    fill_chamfer_rect(panel_x, menu_y + 28, panel_w, 14, XP_LGRAY);
    draw_chamfer_rect(panel_x, menu_y + 28, panel_w, 14, XP_DGRAY);
    draw_compact_string_clipped(panel_x + 6, menu_y + 32, header_chars, "Search apps", XP_DGRAY);

    // Menu items
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
    // The wallpaper covers the whole screen, including the band the dock floats
    // over and the gap beneath it. Stopping it above the dock would leave a hard
    // horizontal seam that gives away the dock as a bar in disguise.
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
    
    // Dock: a floating rounded panel centred on the bottom edge and drawn over
    // the wallpaper, so it reads as its own surface rather than a bar glued to
    // the screen edge.
    dock_layout_t dock;
    dock_compute_layout(&dock);

    // Soft drop shadow, built from concentric outlines so it fades with distance
    // instead of reading as a hard second border.
    for (int s = DOCK_SHADOW; s >= 1; s--) {
        vga_draw_rounded_rect(dock.x - s, dock.y + s, (uint32_t)(dock.w + (s * 2)),
                              (uint32_t)dock.h, (uint8_t)(DOCK_RADIUS + s), XP_SHADOW);
    }
    vga_fill_rounded_rect(dock.x, dock.y, (uint32_t)dock.w, (uint32_t)dock.h,
                          (uint8_t)DOCK_RADIUS, XP_LGRAY);
    vga_draw_rounded_rect(dock.x, dock.y, (uint32_t)dock.w, (uint32_t)dock.h,
                          (uint8_t)DOCK_RADIUS, XP_DGRAY);
    // Bevel highlight along the top edge, inset past the corners.
    fill_rect(dock.x + DOCK_RADIUS, dock.y + 1, dock.w - (DOCK_RADIUS * 2), 1, XP_WHITE);

    // Start button. The flag sits in its own gutter and the label is centred
    // in the space that remains, so the text cannot spill past the border.
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

    // Task buttons, laid out left to right inside the dock. The run width was
    // already reserved by dock_compute_layout, so this loop never overflows.
    int taskbtn_w = dock.taskbtn_w;
    int taskbtn_y = dock.taskbtn_y;
    int task_x = dock.task_x;

    // Longest label that fits inside a task button, minus its side padding.
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
    
    // Clock
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

static int handle_settings_click(window_t* win, int32_t mx, int32_t my) {
    int pane_x = win->x + 4;
    int pane_y = win->y + 28;
    int pane_w = win->w - 8;
    int pane_h = win->h - 32;
  if (mx < pane_x || mx >= pane_x + pane_w || my < pane_y || my >= pane_y + pane_h) {
          return 0;
      }

      // Time zone dropdown, only meaningful on the Time & Language tab.
      if (settings_tab == SETTINGS_TAB_TIME_LANGUAGE) {
          if (handle_settings_tz_click(mx, my)) return 1;
          // A click anywhere else in the content pane dismisses the list, the
          // way a combo box behaves.
          settings_tz_close();
      }


    int nav_x = win->x + 6;
    int nav_y = win->y + 30;
    int nav_w = 124;
    int nav_row_h = 20;
    // Must match draw_window(): the first nav row is drawn at nav_y + 28.
    int nav_start_y = nav_y + 28;
    if (mx >= nav_x + 5 && mx < nav_x + nav_w - 5 &&
        my >= nav_start_y && my < nav_start_y + SETTINGS_TAB_COUNT * nav_row_h) {
        int tab_idx = (my - nav_start_y) / nav_row_h;
        if (tab_idx >= 0 && tab_idx < SETTINGS_TAB_COUNT) {
            settings_tab = tab_idx;
            return 1;
        }
    }

    int content_x = nav_x + nav_w + 8;
    int content_y = win->y + 30;
    int content_w = win->w - (content_x - win->x) - 8;
    (void)content_x;
    (void)content_y;
    (void)content_w;

    // Toggles are registered by whichever tab is on screen, so a single loop
    // covers System, Devices, Network and Accessibility without per-tab
    // geometry duplicated here.
    for (int i = 0; i < settings_tog_count; i++) {
        int ty = settings_tog_hit_y[i];
        if (mx >= settings_tog_hit_x && mx < settings_tog_hit_x + settings_tog_hit_w &&
            my >= ty && my < ty + 12) {
            settings_apply_toggle(settings_tog_id[i]);
            return 1;
        }
    }

    // Segmented control inside GamerOS Update, using the published geometry.
    if (settings_tab == SETTINGS_TAB_GAMEROS_UPDATE && settings_view_hit_h > 0) {
        if (my >= settings_view_hit_y && my < settings_view_hit_y + settings_view_hit_h) {
            for (int seg = 0; seg < 2; seg++) {
                int bx = settings_view_hit_x + seg * (settings_view_hit_w + settings_view_hit_gap);
                if (mx >= bx && mx < bx + settings_view_hit_w) {
                    if (settings_update_view != seg) {
                        settings_update_view = seg;
                        settings_scroll_top[SETTINGS_TAB_GAMEROS_UPDATE] = 0;
                    }
                    return 1;
                }
            }
        }
    }

    if (settings_tab == SETTINGS_TAB_SYSTEM) {
        int list_w = win->w - (settings_res_hit_x - win->x) - 8;
        for (int i = 0; i < settings_res_hit_count; i++) {
            int ry = settings_res_hit_y[i];
            if (mx >= settings_res_hit_x && mx < settings_res_hit_x + list_w &&
                my >= ry && my < ry + settings_res_hit_h) {
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
    return 0;
}

static int handle_explorer_click(window_t* win, int32_t mx, int32_t my) {
    if (!win) return 0;

    // ---- Toolbar buttons (Back / Forward / Up / Refresh) ------------------
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

    // ---- Quick access sidebar ---------------------------------------------
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

    // ---- Entry list --------------------------------------------------------
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

      // The time zone combo scrolls under the wheel before the tab tries
      // anything else, so the list moves instead of the page.
      if (settings_tab == SETTINGS_TAB_TIME_LANGUAGE) {
          if (handle_settings_tz_wheel(mx, my, wheel_delta)) return 1;
      }


    if (settings_tab == SETTINGS_TAB_GAMEROS_UPDATE) {
        settings_md_ensure_loaded();
        // Same box the renderer uses, so the clamp cannot drift from the pixels.
        int sub_y = content_y + 5;
        int sub_h = 15;
        int view_y = sub_y + sub_h + 5;
        int view_h = content_y + content_h - view_y - 12;
        if (view_h < 20) view_h = 20;
        int body_h = view_h - 8;
        if (body_h < 20) body_h = 20;

        int scroll = settings_scroll_top[SETTINGS_TAB_GAMEROS_UPDATE];
        int next = md_scroll_by(content_w - 8, settings_update_view, scroll, -wheel_delta);
        if (next != scroll) {
            settings_scroll_top[SETTINGS_TAB_GAMEROS_UPDATE] = next;
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

        // Dock: same layout the renderer used, so the hit boxes match the pixels.
        dock_layout_t dock;
        dock_compute_layout(&dock);

        // Start button
        if (mx >= dock.start_x && mx < dock.start_x + START_BTN_W &&
            my >= dock.start_y && my < dock.start_y + START_BTN_H) {
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
            if (focused->type == WIN_SETTINGS && handle_settings_click(focused, mx, my)) {
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
        
        // Task buttons
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
    
    // Handle dragging
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
                (void)notepad_app_handle_wheel(wheel_delta);
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

        // Notepad owns its own editing keys.
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

// Main kernel entry
// ============================================================================
// Shell API implementations
//
// These are the only entry points standalone apps have into the shell. They stay
// deliberately thin: the shell owns storage bring-up and GOS:/ path translation,
// so an app never needs to know whether the volume is on ATA or in RAM.
// ============================================================================

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
      // Mount the volume during boot. Deferring this until the first app launch
      // meant a boot log could not tell you whether the medium was detected at
      // all, and made persistence impossible to verify headlessly.
      ensure_storage_initialized();
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

        // Keyboard first: both devices share the PS/2 output buffer, and the
        // mouse drain must never be allowed to consume a keystroke.
        keyboard_poll();
        mouse_poll();
        
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

// ============================================================================
// App Management Stubs (to satisfy linker requirements)
// ============================================================================

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

// ============================================================================
// UI String Helper Stubs (for missing component headers)
// ============================================================================





static const char* settings_ui_panel_title(void) {
    return "Settings";
}

static const char* settings_ui_tab_name(int tab_idx) {
    static const char* tabs[SETTINGS_TAB_COUNT] = {
        "System",
        "Devices",
        "Network",
        "Personalization",
        "Apps",
        "Accounts",
        "Time & Language",
        "Gaming",
        "Accessibility",
        "Privacy",
        "GamerOS Update"
    };
    if (tab_idx < 0 || tab_idx >= SETTINGS_TAB_COUNT) return "Unknown";
    return tabs[tab_idx];
}

static const char* settings_ui_changelog_md_path(void) {
    return "C:/GamerOS/CHANGELOG.md";
}

static const char* settings_ui_changelog_md_text(void) {
    return            "# GamerOS Changelog (2026-10-11)\n"
           "\n"
           "Build 1.550 takes the shell rendered by 1.520 and repairs how text is drawn, then\n"
           "rebuilds the four always-visible surfaces — taskbar, Start menu, desktop watermark\n"
           "and boot screen — on top of that fix.\n"
           "\n"
           "## Release\n"
           "- Version: `00m2`\n"
           "- Build: `1.550` (in progress)\n"
           "- Date: `2026-10-11`\n"
           "- Status: **Unreleased.** Work in progress; still being verified.\n"
           "\n"
           "## Critical — Triple Fault Opening the Start Menu\n"
           "\n"
           "- **Fixed a triple fault that killed the guest the moment the Start button was\n"
           "  clicked.** `boot.asm` enabled `CR4.PAE` but never set `CR4.OSFXSR` (bit 9) or\n"
           "  `CR4.OSXMMEXCPT` (bit 10) before entering long mode. In IA-32e, executing an\n"
           "  SSE instruction with `OSFXSR` clear raises `#UD`. Because the IDT is not\n"
           "  populated for these faults, that `#UD` became `#GP` -> `#DF` -> **triple\n"
           "  fault**, and the CPU shut down. This is what VMware was reporting as\n"
           "  \"a fault has occurred causing a virtual CPU to enter the shutdown state\".\n"
           "- **This was a latent landmine, not a Start-menu bug.** Any GCC-generated SSE\n"
           "  opcode anywhere in the kernel would have killed it. `-O2` emits `pxor` +\n"
           "  `movaps` for any initialisation wider than a 64-bit store, so the moment a\n"
           "  struct crossed 16 bytes the kernel would die. Growing\n"
           "  `start_menu_metrics_t` from 5 to 10 fields pushed it to 40 bytes and tipped GCC\n"
           "  into emitting `pxor %xmm0,%xmm0` at the top of `draw_start_menu()` — which is\n"
           "  why the crash appeared to belong to the Start menu.\n"
           "- **Diagnosed from QEMU's exception trace**, not guessed: `-d int,cpu_reset` gave\n"
           "  the sequence `#UD -> #GP(0x32) -> #DF -> Triple fault` at `RIP=0x109e00`, which\n"
           "  `addr2line` resolved to `draw_start_menu()` at `main.c:2663`, and `info\n"
           "  registers` showed `CR4=0x20` (PAE only, `OSFXSR` clear) against a `pxor`\n"
           "  instruction that cannot fault for any other reason.\n"
           "- **Verified fixed under QEMU 11.0.1:** opening the Start menu now raises **zero\n"
           "  exceptions and zero triple faults**, and the VM stays `running`.\n"
           "- The time zone dropdown and its mouse-wheel scrolling were confirmed working\n"
           "  under VMware. Wheel input could not be exercised through QEMU's monitor at\n"
           "  all — `mouse_button` only exposes left, middle and right, with no wheel\n"
           "  command — so that path was verified on real hardware input instead.\n"
           "\n"
           "## Time Zones\n"
           "\n"
           "- **Added a Time section to Settings, replacing the read-only \"Date & time\"\n"
           "  rows.** The Time & Language tab now shows the local time for the selected zone,\n"
           "  the zone name, its UTC offset, and whether daylight saving is currently in\n"
           "  effect.\n"
           "- **Added a scrollable time zone dropdown with 74 zones.** It is a combo box: a\n"
           "  single row showing the current zone, a `(74 zones)` count and a drop arrow,\n"
           "  which expands to a windowed list of eight rows. The list is longer than the\n"
           "  pane, so the mouse wheel scrolls it; up/down buttons are shown as well and\n"
           "  only when there is somewhere to scroll. Clicking an entry applies it and\n"
           "  closes the list, and clicking anywhere else in the pane dismisses it, the way\n"
           "  a combo box behaves. Row geometry is published by the renderer and read back\n"
           "  by the click and wheel handlers, the same way the other Settings controls stay\n"
           "  aligned.\n"
           "- **The selection changes the clock.** The taskbar clock and the Settings time\n"
           "  now read from the selected zone instead of taking raw RTC time. The RTC is\n"
           "  treated as UTC; local time is UTC plus the zone offset, with daylight saving\n"
           "  applied where the zone observes it. Verified end to end: forcing Kiritimati\n"
           "  (+14:00) at 15:14 UTC produced 05:14 local the following day.\n"
           "- **Daylight saving is implemented, not just offsets.** Four transition rules\n"
           "  are modelled: US/Canada (2nd Sunday March to 1st Sunday November at 02:00\n"
           "  local), European (last Sunday March to last Sunday October at 01:00 UTC),\n"
           "  Australian (1st Sunday October to 1st Sunday April), and New Zealand (last\n"
           "  Sunday September to 1st Sunday April). Zones that do not observe it — Hawaii,\n"
           "  Arizona, Moscow, India, Japan, China — hold a constant offset all year.\n"
           "- **Fixed southern-hemisphere daylight saving covering the wrong months.** The\n"
           "  daylight window spans New Year, and testing against Sydney exposed that\n"
           "  testing \"at or after this year's start OR before next year's end\" marked the\n"
           "  whole April-September gap as daylight time. The window is now anchored to the\n"
           "  previous year whenever the instant falls in the January-April tail.\n"
           "- **Verified with 17 timezone assertions** covering both hemispheres in summer\n"
           "  and winter, non-observing zones holding steady year round, and the US\n"
           "  transitions to the minute: one minute before spring-forward is still EST, and\n"
           "  the transition instant is already EDT.\n"
           "- **The selection persists** to `C:/GamerOS/Registry/TIMEZONE.INI` and is reloaded\n"
           "  at boot, so the clock keeps the chosen zone across a restart.\n"
           "\n"
           "## Correctness\n"
           "\n"
           "- **Fixed `sprintf` having no field-width or zero-padding support.** The parser\n"
           "  jumped straight from `%` to the conversion character, so any specifier with a\n"
           "  width such as `%02u` fell into the default branch and emitted the format text\n"
           "  literally instead of the number. The Time settings rendered\n"
           "  `\\x02u:\\x02u:\\x02u` where the time should have been. `sprintf` now parses a\n"
           "  zero-pad flag and a width, and applies them to `%d`, `%i`, `%u`, `%x`, `%X`,\n"
           "  and `%s`, with the sign counted toward the width for signed conversions.\n"
           "  Anything else in the OS that used `%02u` was silently broken in the same way.\n"
           "- **Fixed the keyboard and mouse starving each other on the PS/2 controller.** The\n"
           "  keyboard and the auxiliary mouse share one output buffer, and status bit 5 says\n"
           "  which device a pending byte belongs to. Both poll loops drained on bit 0 alone,\n"
           "  so `mouse_poll()`, running first each frame, discarded nearly every keystroke\n"
           "  before `keyboard_poll()` saw it: tracing raw scancodes showed one keypress\n"
           "  surviving a 60-key burst. Each loop now stops at the device boundary. Typing\n"
           "  did not work at all before this.\n"
           "- **Fixed Notepad's line-number gutter clipping.** 22px held two digits, so every\n"
           "  number from line 100 up lost its leading digit; it is now 30px for 160 rows.\n"
           "\n"
           "## Application modularisation\n"
           "\n"
           "- **Split Notepad out of `main.c` into its own module.** ~340 lines of editing,\n"
           "  rendering, storage and input now live in `src/ui/apps/notepad/` behind a small\n"
           "  public interface. `src/ui/shell/shell_api.h` defines what an app may ask of the\n"
           "  shell: bring the volume up, test readiness, open or create a `GOS:/` file, raise\n"
           "  an error. Those implementations are thin, so an app never needs to know whether\n"
           "  storage is on ATA or in RAM. The shell keeps window chrome, focus and the input\n"
           "  pump; an app supplies its title, icon, renderer and event handlers and never\n"
           "  touches a `window_t`. This is a source-layout change, not a new on-disk format.\n"
           "  File Explorer and Settings are still embedded in `main.c` and are next.\n"
           "\n"
           "## Build Verification\n"
           "- Bumped the build number from `1.520` to `1.550`, keeping version `00m2`.\n"
           "  Updated in the `GAMEROS.INI` written at boot, the Settings About page, the\n"
           "  desktop watermark, and `README.md`.\n"
           "- Compile-verified: zero warnings, zero errors, zero undefined symbols.\n"
           "- Booted under QEMU and confirmed the desktop shell renders, including the\n"
           "  desktop icons, taskbar, and watermark.\n"
           "- **Storage verified against a real 12 GiB disk** attached on the IDE channel,\n"
           "  across three consecutive boots:\n"
           "  `Storage: ATA medium, capacity 12 GB, LBA48=no, model=QEMU HARDDISK`, then\n"
           "  `files=29 dirs=27 used=21 KB` — identical on every run, proving the volume\n"
           "  reloads from the medium rather than being rebuilt in RAM.\n"
           "- File Explorer was driven through the QEMU monitor to confirm the drive block\n"
           "  and the details columns render, and each captured frame was checked for\n"
           "  exceptions: `triple faults: 0` throughout.\n"
           "- Notepad was verified end to end after being split into its own module: launched\n"
           "  from the desktop icon, typed into, caret moved with the arrow/Home/End keys, the\n"
           "  modified indicator flipping between `Modified` and `Saved`, saved with the\n"
           "  toolbar button, and re-read after a full reboot with the text intact. Storage\n"
           "  stayed at `files=30 dirs=27` across both boots and `triple faults: 0`.\n"
           "\n"
           "## Storage\n"
           "\n"
           "- **The filesystem is now backed by a real disk instead of RAM.** `ata_pio_init()`\n"
           "  had been short-circuited with `g_ata_available = 0; return 0;`, leaving the\n"
           "  real probe as unreachable code, so every boot started from a wiped 2 MiB RAM\n"
           "  disk. The driver is enabled and storage now persists across reboots.\n"
           "- **Rewrote the ATA PIO driver.** It probes all four channel/device pairs rather\n"
           "  than assuming the primary master exists, runs IDENTIFY DEVICE, and reads real\n"
           "  capacity from words 60/61, falling back to the LBA48 words 100-103 when the\n"
           "  28-bit field signals a larger disk. The model and serial strings are decoded\n"
           "  from IDENTIFY words 27-46 and 10-19, correcting for the byte-swapped encoding.\n"
           "- **Implemented LBA48.** Disks above the 28-bit ceiling of 128 GiB need the\n"
           "  extended command set, so reads and writes use `0x24`/`0x34` with the high LBA\n"
           "  byte programmed through the drive/head register when the request exceeds\n"
           "  `0x0FFFFFFF`.\n"
           "- **Fixed the probe handshake that made detection fail.** The driver bailed out\n"
           "  if the drive never asserted DRQ during the post-reset signature phase, which\n"
           "  is legal and which the emulated controller does not do. That single `continue`\n"
           "  was the entire reason detection returned \"no device\". It now logs the failure\n"
           "  and falls through to IDENTIFY regardless.\n"
           "- **The filesystem now starts at LBA 64**, so the MBR, partition table, and any\n"
           "  backup GPT in the first sectors are never written. The superblock used to live\n"
           "  at LBA 0, which would have destroyed the partition table on the first write.\n"
           "- **Capacity is no longer hardcoded.** The old build always reserved\n"
           "  `MAX_DISK_SECTORS 4096` (2 MiB) regardless of the medium and reported a fake\n"
           "  `102400 MB`. The volume now spans whatever the drive reports.\n"
           "- **Fixed files being recreated on every boot.** `MAX_FILENAME_LEN` was 40, and\n"
           "  seeding writes paths such as `C:/GamerOS/Apps/Manifests/Notepad.gosapp`, which\n"
           "  is *exactly* 40 characters. `strncpy(..., MAX_FILENAME_LEN - 1)` stored a\n"
           "  truncated name, so the next boot's full path no longer matched it and\n"
           "  `fs_create_file` appended a duplicate. Six over-length paths meant the volume\n"
           "  gained six junk entries on every single boot, forever. The limit is now 64 and\n"
           "  `fs_create_file` rejects over-long names outright rather than truncating them.\n"
           "  Verified stable at `files=29 dirs=27` across three consecutive boots.\n"
           "- **Stopped reserving 24 KiB of RAM per file.** `file_t` embedded a full\n"
           "  `MAX_FILE_SIZE` buffer, so 32 slots cost 786 KB of BSS to hold data that\n"
           "  belongs on the medium. File contents are read from the medium on demand;\n"
           "  `files[]` is now 18 KB.\n"
           "- **Files are allocated by real size, not a fixed 24 KiB reservation**, so the\n"
           "  usage figure reflects actual occupancy. Added `fs_delete_file()` and\n"
           "  `fs_delete_directory()`, and `fs_get_space_info()` for free/used accounting.\n"
           "- **Raised the volume limits** to 256 files and 128 directories, with the on-disk\n"
           "  tables resized to match and the format version bumped to `GFS3PIO` so any\n"
           "  image from the older layout is cleanly reformatted rather than misread.\n"
           "- **The volume now mounts during boot** instead of being deferred until the first\n"
           "  app launch, so a boot log actually reports whether the medium was detected.\n"
           "\n"
           "## UI\n"
           "\n"
           "- **Turned the taskbar into a floating dock.** A full-width bar glued to the bottom\n"
           "  edge is now a rounded panel inset from the sides and bottom, sized to its\n"
           "  contents and centred, with the wallpaper showing through on all sides. Start,\n"
           "  the task buttons and the clock live inside it, plus a soft drop shadow. The\n"
           "  wallpaper now covers the full screen height, so no seam is left where the dock\n"
           "  used to be, and window placement clamps against the dock's real footprint so the\n"
           "  taller dock never overlaps window bottoms.\n"
           "- Dock geometry comes from one `dock_compute_layout` call read by both the\n"
           "  renderer and the hit tester, so the dock re-centring when windows open or close\n"
           "  cannot leave click targets out of step with the pixels. The Start menu hangs off\n"
           "  the dock's left edge instead of the screen corner.\n"
           "- `vga_fill_rounded_rect` and `vga_draw_rounded_rect` were stubs that ignored\n"
           "  `radius` and drew plain rectangles. They now round the corners for real,\n"
           "  clipping to the screen and writing both the indexed and truecolour buffers.\n"
           "\n"
           "- **Added a Windows-style drive block to Explorer \"This PC\".** Each volume now\n"
           "  shows its model, a usage bar, and a capacity line reading\n"
           "  `11.9 GB free of 12 GB`, in place of a bare `C:` row. The bar is sized from\n"
           "  measured bytes, and volumes under 1 GiB report in MB so a RAM-backed fallback\n"
           "  does not render as `0 GB free`.\n"
           "- **Added the Windows-style details view to File Explorer.** The entry list now\n"
           "  carries `Name | Date modified | Type | Size` with header separators, matching\n"
           "  the Explorer details layout. Name absorbs the leftover width, and the columns\n"
           "  collapse to `Name | Type | Size` when the window is too narrow to show a\n"
           "  readable date.\n"
           "- **Files now carry a modification time.** Added a closed-form Gregorian\n"
           "  date conversion to the RTC driver (`get_unix_time` / `unix_to_calendar`), so\n"
           "  no 64-bit division or `libgcc` helper is needed. `fs_write_file` stamps each\n"
           "  write, and the timestamp is persisted in what used to be padding in the file\n"
           "  record — the record stays 88 bytes, so images written before this field\n"
           "  existed still load (with `mtime` of 0).\n"
           "- **Type is derived from the extension** the way Explorer labels it:\n"
           "  `.EXE` is Application, `.GOSAPP` an Application manifest, `.DLL` an\n"
           "  Application extension, `.INI` Configuration Settings, `.REG` a Registry File,\n"
           "  `.LST` a System File, and folders report as File folder with a blank size.\n"
           "- **Sizes are humanised** to `0 bytes`, `1.2 KB`, `3.4 MB`, computed with\n"
           "  shifts rather than division for the same libgcc reason.\n"
           "\n"
           "- **Fixed the UI text renderer, which was shearing every letter.** `draw_compact_string()`\n"
           "  sampled only the **left 6 of the 8 columns** of each glyph\n"
           "  (`row_data & (0x80 >> col)` for `col` 0-5) while advancing the pen by only 6px.\n"
           "  The right side of every glyph was discarded and the following glyph then\n"
           "  overdrew 2px of it, so compact text rendered as vertical smears across the\n"
           "  whole shell — most visibly on the desktop watermark. It now samples all 8\n"
           "  columns and advances 8px, the font's natural width.\n"
           "- **Converted all 43 hardcoded 6px text-width computations** to `FONT_CW`. Any\n"
           "  site left at 6 would have overflowed its panel by a third once the renderer\n"
           "  started advancing 8. The one non-text use of `/ 6` — the wallpaper gradient\n"
           "  band height — was deliberately left alone.\n"
           "- **Rebuilt the taskbar.** The Start button was 52px wide, but its label needs\n"
           "  40px plus the flag gutter, so the text spilled past the button border. It is\n"
           "  now wide enough, with the label centred in the space left by the flag. The task\n"
           "  strip moved off a hardcoded `x = 56` to a centred run between the Start button\n"
           "  and the clock, which removes the large dead gap on wide displays. Task labels\n"
           "  are now clamped to the button width instead of a fixed 8 characters, and the\n"
           "  Start button shows a pressed state while the menu is open.\n"
           "- **Rebuilt the Start menu.** The profile rail was 42px wide, but \"Admin\" needs\n"
           "  40px at the corrected advance, so the label bled into the content column. The\n"
           "  rail is now 56px, the panel is wider, and the avatar plus both labels are\n"
           "  centred on the rail's own centre line. \"Launchpad\" is placed after the brand\n"
           "  word using a measured gap rather than a magic `x + 74`.\n"
           "- **Centralised the Start menu layout.** `rail_w`, `panel_x`, `panel_w`,\n"
           "  `items_top` and the item count were duplicated between the renderer and the\n"
           "  click handler — the same duplication that previously let click targets drift\n"
           "  away from the pixels they were drawn at. They now come from a single\n"
           "  `get_start_menu_metrics()`. The taskbar strip got the same treatment via\n"
           "  `taskbar_strip_start()`.\n"
           "- **Rebuilt the desktop watermark.** The build badge is right-aligned on the\n"
           "  lower row, and the card is sized from measured text instead of hand-padded\n"
           "  spaces that no longer lined up once the advance changed.\n"
           "- **Rebuilt the loading screen.** The title was centred on the *screen* while\n"
           "  being drawn inside a much narrower panel, so it sat outside the panel on any\n"
           "  wide display; it is now anchored to the panel's content column. The \"Loading\"\n"
           "  label was drawn across the spinner ring at the same `y` and overwrote the dots;\n"
           "  it now sits centred beneath the ring. The panel was also enlarged and centred\n"
           "  rather than positioned from the title's `y`.\n"
           "\n"
           "## Build and Tooling\n"
           "\n"
           "- **Raised the changelog viewer's line budget from 256 to 320** in `main.c`, and\n"
           "  moved the matching cap in `tools/embed_changelog.py` to the same value. The\n"
           "  two were separate hardcoded literals, so raising only one would have left the\n"
           "  generator warning about a truncation the viewer no longer had.\n"
           "\n"
           "## Documentation\n"
           "\n"
           "- **Split the changelog per release.** Build `1.520` (2026-10-10) stays in\n"
           "  `changelog_2026-10-10.md`; this file records build `1.550` (2026-10-11).\n"
           "  `tools/embed_changelog.py` was repointed at this file so the in-OS viewer keeps\n"
           "  showing the newest release.\n"
           "\n"
           "## Notes\n"
           "\n"
           "- The 1.520 notes recorded that the ISO had never been booted. It has now: the\n"
           "  desktop shell, taskbar, and watermark were confirmed rendering under QEMU.\n"
           "- The font fix is global. Every compact string in the OS is 33% wider than it\n"
           "  was, which is why the four surfaces above were rebuilt rather than patched.\n"
           "- Start-menu and taskbar hit boxes are now derived from the same metrics the\n"
           "  renderer uses, so a future layout change cannot silently break clicking.\n"
           "- **Known storage limitation: freed sectors are never reused.** Allocation is\n"
           "  append-only, so deleting a file zeroes its sectors and frees the name, but the\n"
           "  space is not returned to the pool. The volume therefore fills over many write\n"
           "  cycles until it is reformatted. A free-run list or bitmap is needed to close\n"
           "  that gap.\n"
           "- **Only legacy ATA at port 0x1F0 is supported.** AHCI (SATA) and NVMe are not\n"
           "  implemented, so a machine whose disk is on a modern controller will find no\n"
           "  device and fall back to the RAM disk. Under QEMU the default IDE channel works;\n"
           "  VMware VMs commonly default to NVMe or SATA and will need those drivers.\n"
           "\n"
           "## Planned\n"
           "\n"
           "### Kernel\n"
           "\n"
           "- **Populate a real IDT with exception handlers.** `IDT` still reads back as\n"
           "  `0000000000000000 00000000`, so hardware interrupts work only because the\n"
           "  firmware descriptor happens to be in place, and any fault triple-faults\n"
           "  instead of reaching a handler. This is why the `CR4.OSFXSR` bug presented as\n"
           "  an instant shutdown with no diagnostic at all.\n"
           "- Add a panic path that prints the fault vector, error code and the registers\n"
           "  QEMU captured, so a crash is diagnosable from the serial log.\n"
           "- Replace the polled input loop with PIT/HPET timer-driven frame pacing.\n"
           "- Add keyboard shortcuts: Alt+Tab window cycling, a Windows key, and Ctrl+Alt+Del.\n"
           "\n"
           "### Storage\n"
           "\n"
           "- Implement an AHCI (SATA) driver so VMware VMs without a legacy IDE controller\n"
           "  can mount a real volume.\n"
           "- Implement an NVMe driver over PCI for the same reason.\n"
           "- Add a free-sector map so deleted space is reclaimed instead of leaked, and\n"
           "  address the fragmentation the append-only allocator produces over time.\n"
           "- Add a journal so an interrupted metadata write cannot corrupt the volume.\n"
           "- Stop rewriting the whole file table on every write. `fs_save_metadata()`\n"
           "  serialises all 256 records on each `fs_write_file()`, so saving a file costs\n"
           "  the same whether the volume holds three files or three hundred.\n"
           "- Add rename and move, and make `fs_delete_directory()` refuse non-empty\n"
           "  directories instead of deleting the entry and orphaning its children.\n"
           "- Raise the 24 KiB per-file cap in `MAX_FILE_SIZE`, and handle paths that\n"
           "  exceed the 64-character `MAX_FILENAME_LEN` rather than rejecting them.\n"
           "- Add a BIOS-style POST screen so storage detection and capacity are visible\n"
           "  before the desktop loads, instead of only on the serial log.\n"
           "\n"
           "### Shell\n"
           "\n"
           "- Add Explorer sorting by column. The details view draws Name, Date modified,\n"
           "  Type and Size, but only the zone dropdown is interactive so far; the reference\n"
           "  layout shows a sort indicator on the header that does nothing yet.\n"
           "- Expose create, rename and delete in the Explorer UI. The filesystem already\n"
           "  has `fs_create_file`, `fs_delete_file` and `fs_delete_directory`; nothing\n"
           "  calls them.\n"
           "- Give Notepad open/save dialogs instead of the single fixed file path.\n"
           "- Taskbar: window previews on hover, overflow grouping, and show-desktop.\n"
           "- Visually confirm the loading screen; the desktop, taskbar, watermark, Start\n"
           "  menu and Explorer have all been captured as rendered frames.\n"
           "\n"
           "### Settings and time\n"
           "\n"
           "- Persist Settings state so toggles survive a reboot instead of resetting.\n"
           "- Allow the clock to be set. The RTC is read-only today, so the time zone can\n"
           "  only shift the displayed time; nothing corrects a clock that has drifted.\n"
           "- Cover the remaining daylight-saving rules. Iran, Chile, and Lord Howe Island\n"
           "  are approximated by one of the four implemented rules rather than their real\n"
           "  transitions.\n"
           "- Remember each Settings tab's scroll position between visits.\n";
}

