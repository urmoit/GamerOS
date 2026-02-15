#include "../../intf/graphics.h"
#include "../../intf/mouse.h"
#include "../../intf/keyboard.h"
#include "../../intf/serial.h"
#include "../../intf/io.h"
#include "../../intf/ports.h"
#include "../../intf/rtc.h"
#include "../../intf/font.h"
#include "../../intf/fs.h"
#include "stdint.h"
#include "string.h"

// Window states
#define MAX_WINDOWS 6
#define WIN_NONE    0
#define WIN_NOTEPAD 1
#define WIN_MYCOMP  2
#define WIN_ABOUT   3
#define WIN_SETTINGS 4
#define WIN_EXPLORER 5

// Desktop shell sizing (compact XP-like layout for 320x200)
#define TASKBAR_HEIGHT         16
#define START_BTN_X            4
#define START_BTN_W            46
#define START_BTN_H            14
#define DESKTOP_ICON_W         20
#define DESKTOP_ICON_H         20
#define DESKTOP_ICON_HIT_W     28
#define DESKTOP_ICON_HIT_H     32
#define TASKBTN_W              62
#define TASKBTN_H              14
#define START_MENU_X           0
#define START_MENU_W           170
#define START_MENU_H           136
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
#define NOTEPAD_FILE_PATH "C:/Users/Admin/NOTEPAD.TXT"

// Input state
static uint8_t last_buttons = 0;
static uint8_t start_menu_open = 0;
static uint8_t shutdown_requested = 0;
static uint8_t setting_show_seconds = 0;
static uint8_t setting_desktop_glow = 1;
static uint8_t setting_compact_mode = 1;
static int settings_tab = 0;

#define SETTINGS_TAB_SYSTEM 0
#define SETTINGS_TAB_PERSONALIZATION 1
#define SETTINGS_TAB_ACCOUNTS 2
#define SETTINGS_TAB_ABOUT 3
#define SETTINGS_TAB_CHANGELOG 4
#define SETTINGS_TAB_COUNT 5
static int settings_scroll_top[SETTINGS_TAB_COUNT] = {0};

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
static uint8_t cursor_under[CURSOR_SIZE * CURSOR_SIZE];
static int32_t cursor_drawn_x = 0;
static int32_t cursor_drawn_y = 0;
static uint8_t cursor_drawn_valid = 0;

static void append_string(char* dest, size_t dest_cap, const char* src);
static void initialize_storage_layout(void);
static void get_window_min_size(const window_t* win, int* out_w, int* out_h);
static void draw_desktop_watermark(void);

static const char* settings_tabs[] = {
    "System",
    "Personalization",
    "Accounts",
    "About",
    "Changelog"
};

static const char* settings_changelog_lines[] = {
    "GamerOS Changelog 2026-02-15",
    "",
    "Release",
    "Version: 00m1",
    "Build: 1.200",
    "Date: 2026-02-15",
    "",
    "Added",
    "- Storage-backed filesystem integration",
    "- Directory support/list helpers",
    "- File Explorer with This PC drive view",
    "- Tabbed Settings app",
    "- About moved into Settings",
    "- Notepad restored as full app",
    "- Mouse wheel input API",
    "- Notepad load/save/reload actions",
    "- GamerOS System32 namespace layout",
    "- Font size API (8/12/16) added",
    "- RTC date API (day/month/year/weekday)",
    "- Window API: resize/focus/z-order",
    "- Security auth/access baseline model",
    "- User-mode isolated process table",
    "- GUI app mouse/keyboard tab events",
    "- Freestanding sprintf varargs support",
    "",
    "Changed",
    "- Build metadata 1.100 -> 1.200",
    "- About panel uses current graphics mode",
    "- Runtime graphics on VGA mode 12h",
    "- Desktop/window sizing for 640x480",
    "- Storage flow changed to lazy init",
    "- Settings layout refreshed",
    "- Settings content clipping added",
    "- Notepad capacity expanded 20x40->160x96",
    "- Notepad viewport auto-follow",
    "- Taskbar centered geometry + clock box",
    "- Taskbar labels centered from metrics",
    "- Taskbar reserves clock area",
    "- Cursor-only partial presents on move",
    "- Mouse-wheel direction normalized",
    "- Startup screen centered and scaled",
    "- Added GamerOS desktop watermark text",
    "- Windows are now user-resizable",
    "- Executive/GDI init now report failures",
    "- String library expanded: strstr/strncmp/sprintf",
    "- UI window manager now tracks focused window",
    "- sprintf now uses compiler vararg builtins",
    "",
    "Fixed",
    "- VMware hardening across app paths",
    "- Window/input safety edge cases",
    "- Boot/runtime graphics mode mismatch",
    "- Mode 12h planar present instability",
    "- Missing wheel scroll in core apps",
    "- Settings text overflow in frame",
    "- Off-center taskbar render",
    "- Taskbar click-hitbox mismatch",
    "- Cursor jumpiness reduced",
    "- Cursor black trail artifacts fixed",
    "- GUI app no longer relies on timed auto-tab cycle",
    "- Startup path now fails fast on init errors",
    "- Fixed stdarg.h build break in freestanding mode",
    "",
    "Notes",
    "- Explorer remains in safety-first mode",
    "- Mode 12h received multi-pass hardening"
};

static const char* settings_lines[] = {
    "GamerOS Settings",
    "",
    "Display: 320x200 VGA",
    "Theme: XP Classic",
    "Storage Root: C:/",
    "",
    "Toggle options:",
    "1) Taskbar compact mode",
    "2) Desktop glow",
    "3) Clock seconds",
    "",
    "System",
    "Version: 00m1",
    "Build: 1.200"
};

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

        storage_device_info_t dev;
        if (fs_storage_get_device(i, &dev)) {
            append_string(explorer_drive_labels[i], sizeof(explorer_drive_labels[i]), dev.name);
        } else {
            append_string(explorer_drive_labels[i], sizeof(explorer_drive_labels[i]), "Local Disk");
        }
    }
}

static void ensure_storage_initialized(void) {
    if (storage_initialized) return;
    initialize_storage_layout();
    storage_initialized = 1;
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
    fs_init();
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

    fs_create_directory("C:/GamerOS");
    fs_create_directory("C:/GamerOS/System32");
    fs_create_directory("C:/Users");
    fs_create_directory("C:/Users/Admin");
    fs_create_directory("C:/GamerOS/Logs");

    file_t* f = fs_create_file("C:/GamerOS/GAMEROS.INI");
    if (f) {
        const char* txt = "shell=GamerOS\nbuild=1.200\nsystem=C:/GamerOS/System32\n";
        fs_write_file(f, (const uint8_t*)txt, (uint32_t)strlen(txt));
    }
    f = fs_create_file("C:/Users/Admin/README.TXT");
    if (f) {
        const char* txt = "Welcome to GamerOS user profile.\n";
        fs_write_file(f, (const uint8_t*)txt, (uint32_t)strlen(txt));
    }
    f = fs_create_file("C:/Users/Admin/NOTEPAD.TXT");
    if (f) {
        const char* txt = "GamerOS Notepad\n\nType here...\n";
        fs_write_file(f, (const uint8_t*)txt, (uint32_t)strlen(txt));
    }

    f = fs_create_file("C:/GamerOS/System32/NOTEPAD.APP");
    if (f) {
        const char* txt = "App=Notepad\nEntry=WIN_NOTEPAD\n";
        fs_write_file(f, (const uint8_t*)txt, (uint32_t)strlen(txt));
    }
    f = fs_create_file("C:/GamerOS/System32/SETTINGS.APP");
    if (f) {
        const char* txt = "App=Settings\nEntry=WIN_SETTINGS\n";
        fs_write_file(f, (const uint8_t*)txt, (uint32_t)strlen(txt));
    }
    f = fs_create_file("C:/GamerOS/System32/EXPLORER.APP");
    if (f) {
        const char* txt = "App=Explorer\nEntry=WIN_EXPLORER\n";
        fs_write_file(f, (const uint8_t*)txt, (uint32_t)strlen(txt));
    }
    f = fs_create_file("C:/GamerOS/System32/KERNEL.SYS");
    if (f) {
        const char* txt = "Kernel image placeholder\n";
        fs_write_file(f, (const uint8_t*)txt, (uint32_t)strlen(txt));
    }
    f = fs_create_file("C:/GamerOS/System32/SHELL32.DLL");
    if (f) {
        const char* txt = "Shell library placeholder\n";
        fs_write_file(f, (const uint8_t*)txt, (uint32_t)strlen(txt));
    }
    explorer_build_this_pc_entries();
    explorer_open_this_pc();
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

static void draw_desktop_watermark(void) {
    const char* line1 = "GamerOS 00m1 Preview";
    const char* line2 = "Evaluation copy. Build 1.200";
    int w1 = (int)strlen(line1) * 6;
    int w2 = (int)strlen(line2) * 6;
    int max_w = (w1 > w2) ? w1 : w2;
    int x = (int)current_width - max_w - 8;
    int y = (int)current_height - TASKBAR_HEIGHT - 22;
    if (x < 6) x = 6;
    if (y < 6) y = 6;
    draw_compact_string(x, y, line1, XP_LGRAY);
    draw_compact_string(x, y + 9, line2, XP_LGRAY);
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
    file_t* file = fs_open_file(NOTEPAD_FILE_PATH);
    notepad_clear();
    if (!file || file->size == 0) return;

    uint8_t buffer[MAX_FILE_SIZE + 1];
    memset(buffer, 0, sizeof(buffer));
    fs_read_file(file, buffer, MAX_FILE_SIZE);

    int x = 0;
    int y = 0;
    for (uint32_t i = 0; i < MAX_FILE_SIZE && buffer[i]; i++) {
        char c = (char)buffer[i];
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
    file_t* file = fs_open_file(NOTEPAD_FILE_PATH);
    if (!file) file = fs_create_file(NOTEPAD_FILE_PATH);
    if (!file) return;

    uint8_t buffer[MAX_FILE_SIZE];
    memset(buffer, 0, sizeof(buffer));
    uint32_t pos = 0;
    for (int y = 0; y < NOTEPAD_MAX_LINES && pos < MAX_FILE_SIZE - 1; y++) {
        int last = NOTEPAD_MAX_COLS - 1;
        while (last >= 0 && notepad_lines[y][last] == 0) last--;
        for (int x = 0; x <= last && pos < MAX_FILE_SIZE - 1; x++) {
            buffer[pos++] = (uint8_t)notepad_lines[y][x];
        }
        if (y < NOTEPAD_MAX_LINES - 1 && pos < MAX_FILE_SIZE - 1) {
            buffer[pos++] = '\n';
        }
    }
    fs_write_file(file, buffer, pos);
    notepad_dirty = 0;
}

static const char* storage_type_name(storage_device_type_t type) {
    switch (type) {
        case STORAGE_HDD: return "HDD";
        case STORAGE_SSD: return "SSD";
        case STORAGE_NVME: return "NVMe";
        case STORAGE_USB: return "USB";
        case STORAGE_CDROM: return "CDROM";
        case STORAGE_RAMDISK: return "RAM";
        default: return "Unknown";
    }
}

static void startup_animation(void) {
    const char* title = "GamerOS";
    const char* subtitle = "Professional";
    int title_w = (int)strlen(title) * 8;
    int subtitle_w = (int)strlen(subtitle) * 8;
    int bar_w = ((int)current_width * 3) / 5;
    int bar_h = 14;
    if (bar_w < 240) bar_w = 240;
    if (bar_w > 420) bar_w = 420;
    int bar_x = ((int)current_width - bar_w) / 2;
    int bar_y = ((int)current_height / 2) + 26;
    int title_x = ((int)current_width - title_w) / 2;
    int subtitle_x = ((int)current_width - subtitle_w) / 2;
    int title_y = ((int)current_height / 2) - 26;
    int subtitle_y = title_y + 16;

    clear_screen(XP_BLUE);
    draw_string(title_x, title_y, title, XP_WHITE);
    draw_string(subtitle_x, subtitle_y, subtitle, XP_WHITE);
    draw_rect(bar_x, bar_y, bar_w, bar_h, XP_WHITE);
    int fill_target = bar_w - 4;
    int step = fill_target / 120;
    if (step < 2) step = 2;
    for (int p = 0; p < fill_target; p += step) {
        int draw_w = p + step;
        if (draw_w > fill_target) draw_w = fill_target;
        fill_rect(bar_x + 2, bar_y + 2, draw_w, bar_h - 4, XP_LBLUE);
        swap_buffers();
        for (volatile int d = 0; d < 2500; d++) {
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
                strncpy(windows[i].title, "Notepad", 31);
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
                strncpy(windows[i].title, "Settings", 31);
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
    fill_rect(x, y, w, h, XP_WHITE);
    fill_rect(x, y, w, 18, is_active_window ? XP_LBLUE : XP_DGRAY);
    draw_string(x + 4, y + 5, win->title, XP_WHITE);
    
    // Close button
    fill_rect(x + w - 16, y + 2, 14, 14, XP_RED);
    draw_rect(x + w - 16, y + 2, 14, 14, XP_WHITE);
    draw_string(x + w - 12, y + 5, "x", XP_WHITE);
    
    draw_rect(x, y, w, h, XP_BLACK);
    draw_rect(x + 1, y + 1, w - 2, h - 2, XP_LGRAY);
    // Bottom-right resize grip.
    for (int g = 0; g < 4; g++) {
        int gx = x + w - 3 - (g * 3);
        int gy = y + h - 2;
        draw_line(gx, gy, x + w - 2, y + h - 3 - (g * 3), XP_DGRAY);
    }
    
    if (win->type == WIN_NOTEPAD) {
        fill_rect(x + 4, y + 22, w - 8, h - 26, XP_WHITE);
        draw_rect(x + 4, y + 22, w - 8, h - 26, XP_LGRAY);
        fill_rect(x + 5, y + h - 14, w - 10, 8, XP_LGRAY);
        int text_top = y + 26;
        int text_bottom = y + h - 18;
        int visible_lines = (text_bottom - text_top) / 5;
        if (visible_lines < 1) visible_lines = 1;
        if (notepad_view_top < 0) notepad_view_top = 0;
        if (notepad_view_top > NOTEPAD_MAX_LINES - visible_lines) {
            notepad_view_top = NOTEPAD_MAX_LINES - visible_lines;
            if (notepad_view_top < 0) notepad_view_top = 0;
        }
        int max_chars = (w - 16) / 6;
        if (max_chars < 1) max_chars = 1;
        if (max_chars > NOTEPAD_MAX_COLS) max_chars = NOTEPAD_MAX_COLS;
        for (int row = 0; row < visible_lines; row++) {
            int i = notepad_view_top + row;
            if (i >= NOTEPAD_MAX_LINES) break;
            if (notepad_lines[i][0]) {
                draw_compact_string_clipped(x + 8, text_top + row * 5, max_chars, notepad_lines[i], XP_BLACK);
            }
        }
        draw_compact_string(x + 8, y + h - 13, notepad_dirty ? "Modified (PgUp Save, PgDn Reload)" : "Saved (PgUp Save, PgDn Reload)", XP_BLACK);
        if (win == &windows[active_window]) {
            int cx = x + 8 + notepad_cursor_x * 6;
            int cy = text_top + (notepad_cursor_y - notepad_view_top) * 5;
            if (cy >= text_top && cy < text_bottom) {
                fill_rect(cx, cy, 5, 6, XP_BLACK);
            }
        }
    } else if (win->type == WIN_SETTINGS) {
        fill_rect(x + 4, y + 22, w - 8, h - 26, XP_WHITE);
        draw_rect(x + 4, y + 22, w - 8, h - 26, XP_LGRAY);
        int nav_x = x + 6;
        int nav_y = y + 24;
        int nav_w = 108;
        int nav_h = h - 30;
        fill_rect(nav_x, nav_y, nav_w, nav_h, XP_LGRAY);
        draw_rect(nav_x, nav_y, nav_w, nav_h, XP_WHITE);
        fill_rect(nav_x + 1, nav_y + 1, nav_w - 2, 12, XP_BLUE);
        draw_compact_string(nav_x + 6, nav_y + 3, "Settings", XP_WHITE);

        int content_x = nav_x + nav_w + 6;
        int content_y = y + 24;
        int content_w = w - (content_x - x) - 6;
        int content_h = h - 30;
        fill_rect(content_x, content_y, content_w, content_h, XP_WHITE);
        draw_rect(content_x, content_y, content_w, content_h, XP_LGRAY);
        int content_chars = (content_w - 12) / 6;
        if (content_chars < 1) content_chars = 1;

        for (int i = 0; i < SETTINGS_TAB_COUNT; i++) {
            int ty = nav_y + 20 + i * 16;
            fill_rect(nav_x + 3, ty - 1, nav_w - 6, 13, (settings_tab == i) ? XP_LBLUE : XP_WHITE);
            draw_rect(nav_x + 3, ty - 1, nav_w - 6, 13, XP_DGRAY);
            if (settings_tab == i) {
                draw_rect(nav_x + 4, ty, nav_w - 8, 11, XP_WHITE);
            }
            draw_compact_string_clipped(nav_x + 8, ty + 2, (nav_w - 14) / 6, settings_tabs[i], XP_BLACK);
        }

        if (settings_tab == SETTINGS_TAB_SYSTEM) {
            char opt1[40];
            char opt2[40];
            char opt3[40];
            strncpy(opt1, "1) Taskbar compact: ", sizeof(opt1) - 1);
            opt1[sizeof(opt1) - 1] = 0;
            append_string(opt1, sizeof(opt1), setting_compact_mode ? "On" : "Off");
            strncpy(opt2, "2) Desktop glow: ", sizeof(opt2) - 1);
            opt2[sizeof(opt2) - 1] = 0;
            append_string(opt2, sizeof(opt2), setting_desktop_glow ? "On" : "Off");
            strncpy(opt3, "3) Clock seconds: ", sizeof(opt3) - 1);
            opt3[sizeof(opt3) - 1] = 0;
            append_string(opt3, sizeof(opt3), setting_show_seconds ? "On" : "Off");
            draw_compact_string_clipped(content_x + 6, content_y + 6, content_chars, "System Controls", XP_BLACK);
            draw_compact_string_clipped(content_x + 6, content_y + 22, content_chars, opt1, XP_BLACK);
            draw_compact_string_clipped(content_x + 6, content_y + 34, content_chars, opt2, XP_BLACK);
            draw_compact_string_clipped(content_x + 6, content_y + 46, content_chars, opt3, XP_BLACK);
            draw_compact_string_clipped(content_x + 6, content_y + 60, content_chars, "Storage devices:", XP_BLACK);
            int sc = fs_storage_get_device_count();
            char storage_line[44];
            strncpy(storage_line, "Detected profiles: ", sizeof(storage_line) - 1);
            storage_line[sizeof(storage_line) - 1] = 0;
            if (sc > 9) sc = 9;
            char c[2];
            c[0] = (char)('0' + sc);
            c[1] = 0;
            append_string(storage_line, sizeof(storage_line), c);
            draw_compact_string_clipped(content_x + 6, content_y + 70, content_chars, storage_line, XP_BLACK);
            draw_compact_string_clipped(content_x + 6, content_y + 78, content_chars, "HDD SSD NVMe USB CDROM RAM", XP_BLACK);
        } else if (settings_tab == SETTINGS_TAB_PERSONALIZATION) {
            draw_compact_string_clipped(content_x + 6, content_y + 6, content_chars, "Personalization", XP_BLACK);
            draw_compact_string_clipped(content_x + 6, content_y + 22, content_chars, "Theme: XP Classic", XP_BLACK);
            draw_compact_string_clipped(content_x + 6, content_y + 34, content_chars, "Desktop: 640x480", XP_BLACK);
            draw_compact_string_clipped(content_x + 6, content_y + 46, content_chars, "Color profile: Blue", XP_BLACK);
        } else if (settings_tab == SETTINGS_TAB_ACCOUNTS) {
            draw_compact_string_clipped(content_x + 6, content_y + 6, content_chars, "Accounts", XP_BLACK);
            draw_compact_string_clipped(content_x + 6, content_y + 22, content_chars, "Current user: Admin", XP_BLACK);
            draw_compact_string_clipped(content_x + 6, content_y + 34, content_chars, "Profile: C:/Users/Admin", XP_BLACK);
            draw_compact_string_clipped(content_x + 6, content_y + 46, content_chars, "Mode: Local", XP_BLACK);
        } else if (settings_tab == SETTINGS_TAB_ABOUT) {
            draw_compact_string_clipped(content_x + 6, content_y + 6, content_chars, "About GamerOS", XP_BLACK);
            draw_compact_string_clipped(content_x + 6, content_y + 22, content_chars, "Version: 00m1", XP_BLACK);
            draw_compact_string_clipped(content_x + 6, content_y + 34, content_chars, "Build: 1.200", XP_BLACK);
            draw_compact_string_clipped(content_x + 6, content_y + 46, content_chars, "Kernel: x86_64", XP_BLACK);
            draw_compact_string_clipped(content_x + 6, content_y + 58, content_chars, "Graphics: VGA 640x480x16", XP_BLACK);
            draw_compact_string_clipped(content_x + 6, content_y + 70, content_chars, "System32: C:/GamerOS/System32", XP_BLACK);
            draw_compact_string_clipped(content_x + 6, content_y + 82, content_chars, "Apps: NOTEPAD/SETTINGS/EXPLORER", XP_BLACK);
            draw_compact_string_clipped(content_x + 6, content_y + 94, content_chars, "Author: Chosentechies", XP_BLACK);
        } else if (settings_tab == SETTINGS_TAB_CHANGELOG) {
            int total = (int)(sizeof(settings_changelog_lines) / sizeof(settings_changelog_lines[0]));
            int row_h = 10;
            int visible = (content_h - 20) / row_h;
            if (visible < 1) visible = 1;
            int top = settings_scroll_top[SETTINGS_TAB_CHANGELOG];
            if (top < 0) top = 0;
            if (top > total - visible) top = total - visible;
            if (top < 0) top = 0;
            settings_scroll_top[SETTINGS_TAB_CHANGELOG] = top;
            for (int r = 0; r < visible; r++) {
                int idx = top + r;
                if (idx >= total) break;
                draw_compact_string_clipped(content_x + 6, content_y + 8 + r * row_h, content_chars, settings_changelog_lines[idx], XP_BLACK);
            }
            draw_compact_string_clipped(content_x + 6, content_y + content_h - 10, content_chars, "Mouse wheel: scroll changelog", XP_DGRAY);
        }
    } else if (win->type == WIN_EXPLORER) {
        fill_rect(x + 4, y + 22, w - 8, h - 26, XP_WHITE);
        draw_rect(x + 4, y + 22, w - 8, h - 26, XP_LGRAY);

        fill_rect(x + 6, y + 24, w - 12, 12, XP_LBLUE);
        draw_compact_string(x + 10, y + 26, "This PC", XP_WHITE);
        draw_compact_string(x + 10, y + 42, "Explorer Safe Mode", XP_BLACK);
        draw_compact_string(x + 10, y + 54, "Drive view temporarily", XP_BLACK);
        draw_compact_string(x + 10, y + 66, "limited for VMware stability.", XP_BLACK);
        draw_compact_string(x + 10, y + 82, "C: Local Disk", XP_BLACK);
        draw_compact_string(x + 10, y + 94, "D: Data", XP_BLACK);
    }
}

// Desktop icons
typedef struct {
    int x, y;
    const char* label;
    int type;
} desktop_icon_t;

static desktop_icon_t desktop_icons[] = {
    {18, 18, "Notepad", WIN_NOTEPAD},
    {18, 62, "Settings", WIN_SETTINGS},
    {18, 106, "Explorer", WIN_EXPLORER},
};
#define NUM_ICONS (sizeof(desktop_icons) / sizeof(desktop_icons[0]))

static void draw_notepad_icon(int x, int y) {
    // Paper body
    fill_rect(x, y, DESKTOP_ICON_W, DESKTOP_ICON_H, XP_WHITE);
    draw_rect(x, y, DESKTOP_ICON_W, DESKTOP_ICON_H, XP_LBLUE);
    // Folded corner
    fill_rect(x + DESKTOP_ICON_W - 5, y, 5, 5, XP_LGRAY);
    draw_line(x + DESKTOP_ICON_W - 5, y + 4, x + DESKTOP_ICON_W - 1, y, XP_DGRAY);
    // Blue writing lines
    fill_rect(x + 4, y + 7, DESKTOP_ICON_W - 8, 1, XP_BLUE);
    fill_rect(x + 4, y + 11, DESKTOP_ICON_W - 8, 1, XP_BLUE);
    fill_rect(x + 4, y + 15, DESKTOP_ICON_W - 8, 1, XP_BLUE);
}

static void draw_settings_icon(int x, int y) {
    fill_rect(x, y, DESKTOP_ICON_W, DESKTOP_ICON_H, XP_LGRAY);
    draw_rect(x, y, DESKTOP_ICON_W, DESKTOP_ICON_H, XP_WHITE);
    fill_rect(x + 8, y + 4, 4, 12, XP_DGRAY);
    fill_rect(x + 4, y + 8, 12, 4, XP_DGRAY);
    fill_rect(x + 6, y + 6, 8, 8, XP_BLUE);
    fill_rect(x + 8, y + 8, 4, 4, XP_WHITE);
}

static void draw_explorer_icon(int x, int y) {
    fill_rect(x, y + 5, DESKTOP_ICON_W, DESKTOP_ICON_H - 5, 0x2A);
    fill_rect(x + 2, y + 2, 10, 5, 0x3D);
    draw_rect(x, y + 5, DESKTOP_ICON_W, DESKTOP_ICON_H - 5, XP_WHITE);
}

static void draw_desktop_icon(int x, int y, int type) {
    if (type == WIN_SETTINGS) {
        draw_settings_icon(x, y);
        return;
    } else if (type == WIN_EXPLORER) {
        draw_explorer_icon(x, y);
        return;
    }
    draw_notepad_icon(x, y);
}

static void draw_start_menu(void) {
    int menu_x = START_MENU_X;
    int menu_w = START_MENU_W;
    int menu_h = START_MENU_H;
    int menu_y = (int)current_height - TASKBAR_HEIGHT - START_MENU_H;
    int item_h = START_MENU_ITEM_H;

    // Outer shell
    fill_rect(menu_x, menu_y, menu_w, menu_h, XP_LGRAY);
    draw_rect(menu_x, menu_y, menu_w, menu_h, XP_WHITE);
    draw_rect(menu_x + 1, menu_y + 1, menu_w - 2, menu_h - 2, XP_BLUE);

    // Left user pane
    fill_rect(menu_x + 4, menu_y + 4, 38, menu_h - 8, XP_BLUE);
    draw_string(menu_x + 10, menu_y + 10, "XP", XP_WHITE);

    // Header bar
    fill_rect(menu_x + 44, menu_y + 4, menu_w - 48, 18, XP_LBLUE);
    draw_string(menu_x + 48, menu_y + 9, "GamerOS", XP_WHITE);

    // Menu items
    fill_rect(menu_x + 46, menu_y + 28, menu_w - 52, item_h - 2, XP_WHITE);
    draw_string(menu_x + 52, menu_y + 34, "Notepad", XP_BLACK);

    fill_rect(menu_x + 46, menu_y + 28 + item_h, menu_w - 52, item_h - 2, XP_LGRAY);
    draw_string(menu_x + 52, menu_y + 34 + item_h, "Settings", XP_BLACK);

    fill_rect(menu_x + 46, menu_y + 28 + (item_h * 2), menu_w - 52, item_h - 2, XP_LGRAY);
    draw_string(menu_x + 52, menu_y + 34 + (item_h * 2), "File Explorer", XP_BLACK);

    fill_rect(menu_x + 44, menu_y + menu_h - 24, menu_w - 48, 16, XP_DGRAY);
    draw_string(menu_x + 50, menu_y + menu_h - 20, "Shut Down", XP_WHITE);
}

void draw_desktop(void) {
    fill_rect(0, 0, current_width, current_height - TASKBAR_HEIGHT, 0x39);
    if (setting_desktop_glow) {
        fill_rect(0, current_height - TASKBAR_HEIGHT - 14, current_width, 14, XP_LBLUE);
    }
    
    for (int i = 0; i < (int)NUM_ICONS; i++) {
        int x = desktop_icons[i].x;
        int y = desktop_icons[i].y;
        draw_desktop_icon(x, y, desktop_icons[i].type);
        int label_w = (int)strlen(desktop_icons[i].label) * 6;
        int label_x = x + (DESKTOP_ICON_W / 2) - (label_w / 2);
        draw_compact_string(label_x, y + DESKTOP_ICON_H + 3, desktop_icons[i].label, XP_WHITE);
    }
    
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].active) {
            draw_window(&windows[i]);
        }
    }

    draw_desktop_watermark();
    
    // Taskbar
    int taskbar_y = (int)current_height - TASKBAR_HEIGHT;
    int start_y = taskbar_y + ((TASKBAR_HEIGHT - START_BTN_H) / 2);
    int taskbtn_y = taskbar_y + ((TASKBAR_HEIGHT - TASKBTN_H) / 2);
    fill_rect(0, taskbar_y, current_width, TASKBAR_HEIGHT, XP_BLUE);
    draw_rect(0, taskbar_y, current_width, TASKBAR_HEIGHT, XP_WHITE);

    // Start button (center text inside the button using compact font for consistent metrics)
    fill_rect(START_BTN_X, start_y, START_BTN_W, START_BTN_H, XP_GREEN);
    draw_rect(START_BTN_X, start_y, START_BTN_W, START_BTN_H, XP_WHITE);
    {
        const char* start_label = "Start";
        int start_text_w = (int)strlen(start_label) * 6;
        int start_text_x = START_BTN_X + (START_BTN_W - start_text_w) / 2;
        int start_text_y = start_y + (START_BTN_H - 8) / 2;
        draw_compact_string(start_text_x, start_text_y, start_label, XP_WHITE);
    }

    if (start_menu_open) {
        draw_start_menu();
    }
    
    // Taskbar items
    int taskbtn_w = setting_compact_mode ? TASKBTN_W : (TASKBTN_W + 16);
    int task_x = 56;
    int clock_chars = setting_show_seconds ? 8 : 5;
    int clock_w = (clock_chars * 6) + 8;
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
                fill_rect(task_x, taskbtn_y, taskbtn_w, TASKBTN_H, XP_LBLUE);
            } else {
                fill_rect(task_x, taskbtn_y, taskbtn_w, TASKBTN_H, XP_BLUE);
            }
            draw_rect(task_x, taskbtn_y, taskbtn_w, TASKBTN_H, XP_WHITE);

            char task_text[9];
            size_t len = strlen(windows[i].title);
            if (len > 8) len = 8;
            strncpy(task_text, windows[i].title, len);
            task_text[len] = 0;
            int tx = task_x + (taskbtn_w - ((int)len * 6)) / 2;
            int ty = taskbtn_y + (TASKBTN_H - 8) / 2;
            draw_compact_string(tx, ty, task_text, XP_WHITE);
            task_x += taskbtn_w + 2;
        }
    }
    
    // Clock
    uint8_t hour = 0, minute = 0, second = 0;
    get_time(&hour, &minute, &second);
    char time_str[9];
    time_str[0] = (char)('0' + ((hour / 10) % 10));
    time_str[1] = (char)('0' + (hour % 10));
    time_str[2] = ':';
    time_str[3] = (char)('0' + ((minute / 10) % 10));
    time_str[4] = (char)('0' + (minute % 10));
    if (setting_show_seconds) {
        time_str[5] = ':';
        time_str[6] = (char)('0' + ((second / 10) % 10));
        time_str[7] = (char)('0' + (second % 10));
        time_str[8] = 0;
    } else {
        time_str[5] = 0;
    }
    fill_rect(clock_x, clock_y, clock_w, clock_h, XP_LBLUE);
    draw_rect(clock_x, clock_y, clock_w, clock_h, XP_WHITE);
    {
        int clock_text_w = ((int)strlen(time_str)) * 6;
        int clock_text_x = clock_x + (clock_w - clock_text_w) / 2;
        int clock_text_y = clock_y + (clock_h - 8) / 2;
        draw_compact_string(clock_text_x, clock_text_y, time_str, XP_WHITE);
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
    int content_y = win->y + 22;
    int content_w = win->w - 8;
    int content_h = win->h - 26;
    if (mx < content_x || mx >= content_x + content_w || my < content_y || my >= content_y + content_h) {
        return 0;
    }

    int nav_x = win->x + 6;
    int nav_y = win->y + 24;
    int nav_w = 108;
    int nav_start_y = nav_y + 20;
    if (mx >= nav_x + 3 && mx < nav_x + nav_w - 3 &&
        my >= nav_start_y && my < nav_start_y + SETTINGS_TAB_COUNT * 16) {
        int tab_idx = (my - nav_start_y) / 16;
        if (tab_idx >= 0 && tab_idx < SETTINGS_TAB_COUNT) {
            settings_tab = tab_idx;
            return 1;
        }
    }

    if (settings_tab == SETTINGS_TAB_SYSTEM) {
        int content_x = nav_x + nav_w + 6;
        int content_y = win->y + 24;
        int rel_x = mx - (content_x + 6);
        int rel_y = my - (content_y + 22);
        if (rel_x >= 0 && rel_x < 150) {
            if (rel_y >= 0 && rel_y < 8) {
                setting_compact_mode = (uint8_t)!setting_compact_mode;
                return 1;
            }
            if (rel_y >= 12 && rel_y < 20) {
                setting_desktop_glow = (uint8_t)!setting_desktop_glow;
                return 1;
            }
            if (rel_y >= 24 && rel_y < 32) {
                setting_show_seconds = (uint8_t)!setting_show_seconds;
                return 1;
            }
        }
    }
    return 0;
}

static int handle_explorer_click(window_t* win, int32_t mx, int32_t my) {
    (void)win;
    (void)mx;
    (void)my;
    // Safe mode: no dynamic actions until VMware fault source is fully isolated.
    return 1;
}

static int handle_notepad_wheel(window_t* win, int8_t wheel_delta) {
    if (!win || wheel_delta == 0) return 0;
    int text_top = win->y + 26;
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
    int nav_w = 108;
    int content_x = nav_x + nav_w + 6;
    int content_y = win->y + 24;
    int content_w = win->w - (content_x - win->x) - 6;
    int content_h = win->h - 30;
    if (mx < content_x || mx >= content_x + content_w || my < content_y || my >= content_y + content_h) {
        return 0;
    }

    if (settings_tab == SETTINGS_TAB_CHANGELOG) {
        int total = (int)(sizeof(settings_changelog_lines) / sizeof(settings_changelog_lines[0]));
        int visible = (content_h - 20) / 10;
        if (visible < 1) visible = 1;
        int top = settings_scroll_top[SETTINGS_TAB_CHANGELOG] - (int)wheel_delta;
        if (top < 0) top = 0;
        if (top > total - visible) top = total - visible;
        if (top < 0) top = 0;
        if (top != settings_scroll_top[SETTINGS_TAB_CHANGELOG]) {
            settings_scroll_top[SETTINGS_TAB_CHANGELOG] = top;
            return 1;
        }
    }
    return 0;
}

// Process mouse input
void process_mouse(int32_t mx, int32_t my, uint8_t buttons, int8_t wheel_delta) {
#define RETURN_MOUSE() do { last_buttons = buttons; return; } while (0)
    uint8_t pressed = buttons & ~last_buttons;
    
    if (pressed & MOUSE_BTN_LEFT) {
        // Start button
        int start_y = (int)current_height - TASKBAR_HEIGHT + ((TASKBAR_HEIGHT - START_BTN_H) / 2);
        if (mx >= START_BTN_X && mx < START_BTN_X + START_BTN_W &&
            my >= start_y && my < start_y + START_BTN_H) {
            start_menu_open = !start_menu_open;
            RETURN_MOUSE();
        }

        // Start menu items
        if (start_menu_open) {
            int menu_x = START_MENU_X;
            int menu_w = START_MENU_W;
            int menu_h = START_MENU_H;
            int menu_y = (int)current_height - TASKBAR_HEIGHT - menu_h;
            int menu_item_h = START_MENU_ITEM_H;
            if (mx >= menu_x && mx < menu_x + menu_w && my >= menu_y && my < menu_y + menu_h) {
                int rel_y = my - (menu_y + 28);
                if (rel_y >= 0 && rel_y < menu_item_h) {
                    open_window(WIN_NOTEPAD, 50, 28);
                } else if (rel_y >= menu_item_h && rel_y < (menu_item_h * 2)) {
                    open_window(WIN_SETTINGS, 50, 24);
                } else if (rel_y >= (menu_item_h * 2) && rel_y < (menu_item_h * 3)) {
                    open_window(WIN_EXPLORER, 42, 28);
                } else if (my >= menu_y + menu_h - 24 && my < menu_y + menu_h - 8 &&
                           mx >= menu_x + 44 && mx < menu_x + menu_w - 4) {
                    shutdown_os();
                }
                start_menu_open = 0;
                RETURN_MOUSE();
            }
            start_menu_open = 0;
        }

        int clicked_idx = find_top_window_at(mx, my);
        if (clicked_idx >= 0) {
            int tx = windows[clicked_idx].x;
            int ty = windows[clicked_idx].y;
            int tw = windows[clicked_idx].w;
            int th = windows[clicked_idx].h;

            if (mx >= tx + tw - 16 && mx < tx + tw && my >= ty + 2 && my < ty + 16) {
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

            if (mx >= tx && mx < tx + tw - 16 && my >= ty && my < ty + 18) {
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
                open_window(desktop_icons[i].type, 60 + i * 30, 40 + i * 20);
                RETURN_MOUSE();
            }
        }
        
        // Check taskbar
        int taskbar_y = (int)current_height - TASKBAR_HEIGHT;
        int taskbtn_y = taskbar_y + ((TASKBAR_HEIGHT - TASKBTN_H) / 2);
        int taskbtn_w = setting_compact_mode ? TASKBTN_W : (TASKBTN_W + 16);
        int task_x = 56;
        int clock_chars = setting_show_seconds ? 8 : 5;
        int clock_w = (clock_chars * 6) + 8;
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
        for (int i = 0; i < MAX_WINDOWS; i++) {
            if (windows[i].active && windows[i].dragging) {
                windows[i].x = mx - windows[i].drag_x;
                windows[i].y = my - windows[i].drag_y;
                if (windows[i].x < 0) windows[i].x = 0;
                if (windows[i].y < 0) windows[i].y = 0;
                if (windows[i].x > (int)current_width - windows[i].w) windows[i].x = current_width - windows[i].w;
                if (windows[i].y > (int)current_height - TASKBAR_HEIGHT - 16) {
                    windows[i].y = (int)current_height - TASKBAR_HEIGHT - 16;
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
                if (storage_initialized) {
                    notepad_save_to_storage();
                }
                changed = 1;
            } else if ((uint8_t)c == KEY_PGDN) {
                if (storage_initialized) {
                    notepad_load_from_storage();
                }
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
                int text_top = win->y + 26;
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
            draw_pixel(cursor_drawn_x + col, cursor_drawn_y + row, cursor_under[idx]);
        }
    }
}

static void cursor_capture_background(int32_t x, int32_t y) {
    for (int row = 0; row < CURSOR_SIZE; row++) {
        for (int col = 0; col < CURSOR_SIZE; col++) {
            int idx = row * CURSOR_SIZE + col;
            cursor_under[idx] = vga_get_pixel(x + col, y + row);
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
void kernel_main(void) {
    serial_init();
    serial_write_string("GamerOS Starting...\n");
    
    // Set graphics mode (mode 12h pipeline in long mode).
    set_video_mode(MODE_VESA_640x480);
    
    // Initialize subsystems
    keyboard_init();
    mouse_init();
    // Keep startup path minimal; initialize storage lazily on first app use.
    explorer_drive_count = 1;
    explorer_drive_paths[0][0] = 'C';
    explorer_drive_paths[0][1] = ':';
    explorer_drive_paths[0][2] = '/';
    explorer_drive_paths[0][3] = 0;
    strncpy(explorer_drive_labels[0], "C: Local Disk", sizeof(explorer_drive_labels[0]) - 1);
    explorer_drive_labels[0][sizeof(explorer_drive_labels[0]) - 1] = 0;
    init_windows();
    startup_animation();

    // VMware stability mode: use polling-only input in the main loop.
    // IRQ-driven mouse/keyboard paths can still trigger fault storms while dragging.
    __asm__ volatile ("cli");
    
    // Main loop
    int32_t last_mx = -1, last_my = -1;
    uint8_t scene_dirty = 1;
    
    while (1) {
        if (shutdown_requested) {
            // Stay on shutdown screen without triggering VMware "CPU disabled" popup.
            for (;;) {
                __asm__ volatile ("pause");
            }
        }

        mouse_poll();
        keyboard_poll();
        
        int32_t mx = mouse_get_x();
        int32_t my = mouse_get_y();
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
    }
}

// TODO: Replace polling-only desktop loop with timer-driven frame pacing to reduce idle CPU usage.
// TODO: Derive scene_dirty from centralized event queue instead of scattered state checks.
