#include "../../intf/graphics.h"
#include "../../intf/mouse.h"
#include "../../intf/keyboard.h"
#include "../../intf/serial.h"
#include "../../intf/io.h"
#include "../../intf/ports.h"
#include "../../intf/rtc.h"
#include "../../intf/font.h"
#include "../../intf/fs.h"
#include "../../intf/apps.h"
#include "../../apps/notepad/notepad_ui.h"
#include "../../apps/settings/settings_ui.h"
#include "../../apps/explorer/explorer_ui.h"
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
#define SETTINGS_TAB_ABOUT 11
#define SETTINGS_TAB_COUNT 12
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
static uint8_t notepad_io_buffer[MAX_FILE_SIZE + 1];
static int32_t cursor_drawn_x = 0;
static int32_t cursor_drawn_y = 0;
static uint8_t cursor_drawn_valid = 0;

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

static void append_string(char* dest, size_t dest_cap, const char* src);
static void initialize_storage_layout(void);
static void get_window_min_size(const window_t* win, int* out_w, int* out_h);
static void draw_desktop_watermark(void);
static void draw_desktop_wallpaper(int desktop_h);
static int get_ui_scale(void);
static void get_start_menu_metrics(start_menu_metrics_t* m);
static void launch_application_exe(const char* exe_name, int fallback_x, int fallback_y);
static void launch_settings_tab(int tab_idx);
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
    settings_md_loaded = 0;
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
    fs_create_directory("C:/GamerOS/Apps");
    fs_create_directory("C:/GamerOS/Apps/Settings");
    fs_create_directory("C:/GamerOS/Apps/BuiltIn");
    fs_create_directory("C:/Users");
    fs_create_directory("C:/Users/Admin");
    fs_create_directory("C:/GamerOS/Logs");

    file_t* f = fs_create_file("C:/GamerOS/GAMEROS.INI");
    if (f) {
        const char* txt = "shell=GamerOS\nbuild=1.300\nsystem=C:/GamerOS/System32\napps=C:/GamerOS/Apps\n";
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

    f = fs_create_file("C:/GamerOS/System32/NOTEPAD.EXE");
    if (f) {
        const char* txt = "MZ\nName=Notepad\nSubsystem=GamerOS\nEntry=WIN_NOTEPAD\n";
        fs_write_file(f, (const uint8_t*)txt, (uint32_t)strlen(txt));
    }
    f = fs_create_file("C:/GamerOS/System32/SETTINGS.EXE");
    if (f) {
        const char* txt = "MZ\nName=Settings\nSubsystem=GamerOS\nEntry=WIN_SETTINGS\n";
        fs_write_file(f, (const uint8_t*)txt, (uint32_t)strlen(txt));
    }
    f = fs_create_file("C:/GamerOS/System32/EXPLORER.EXE");
    if (f) {
        const char* txt = "MZ\nName=Explorer\nSubsystem=GamerOS\nEntry=WIN_EXPLORER\n";
        fs_write_file(f, (const uint8_t*)txt, (uint32_t)strlen(txt));
    }
    f = fs_create_file("C:/GamerOS/Apps/BuiltIn/APPS.LST");
    if (f) {
        const char* txt = "NOTEPAD.EXE\nSETTINGS.EXE\nEXPLORER.EXE\n";
        fs_write_file(f, (const uint8_t*)txt, (uint32_t)strlen(txt));
    }
    f = fs_create_file(settings_ui_changelog_md_path());
    if (f) {
        const char* txt = settings_ui_changelog_md_text();
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
    const char* line1 = "GamerOS 00m1 Aero Preview";
    const char* line2 = "Windows 7 Style UI - Build 1.300";
    int w1 = (int)strlen(line1) * 6;
    int w2 = (int)strlen(line2) * 6;
    int max_w = (w1 > w2) ? w1 : w2;
    int x = (int)current_width - max_w - 8;
    int y = (int)current_height - TASKBAR_HEIGHT - 22;
    if (x < 6) x = 6;
    if (y < 6) y = 6;
    draw_compact_string(x, y, line1, XP_WHITE);
    draw_compact_string(x, y + 9, line2, XP_LGRAY);
}

static void draw_desktop_wallpaper(int desktop_h) {
    if (desktop_h <= 0) return;
    if (background_wallpaper_width == 0 || background_wallpaper_height == 0) {
        fill_rect(0, 0, (int)current_width, desktop_h, XP_BLUE);
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
    int item_h = 16;
    if (menu_w > (int)current_width - 8) menu_w = (int)current_width - 8;
    if (menu_h > (int)current_height - TASKBAR_HEIGHT - 4) menu_h = (int)current_height - TASKBAR_HEIGHT - 4;
    m->x = START_MENU_X;
    m->w = menu_w;
    m->h = menu_h;
    m->item_h = item_h;
    m->y = (int)current_height - TASKBAR_HEIGHT - menu_h;
}

static void launch_application_exe(const char* exe_name, int fallback_x, int fallback_y) {
    int win_type = WIN_NONE;
    int launch_x = fallback_x;
    int launch_y = fallback_y;
    if (!apps_resolve_launch(exe_name, &win_type, &launch_x, &launch_y)) {
        return;
    }
    if (fallback_x >= 0) launch_x = fallback_x;
    if (fallback_y >= 0) launch_y = fallback_y;

    open_window(win_type, launch_x, launch_y);
    if (win_type == WIN_NOTEPAD && storage_initialized) {
        notepad_load_from_storage();
    }
}

static void launch_settings_tab(int tab_idx) {
    launch_application_exe("SETTINGS.EXE", -1, -1);
    if (tab_idx >= 0 && tab_idx < SETTINGS_TAB_COUNT) {
        settings_tab = tab_idx;
    }
}

static void settings_md_reset(void) {
    settings_md_line_count = 0;
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
    file_t* file = fs_open_file(NOTEPAD_FILE_PATH);
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
    file_t* file = fs_open_file(NOTEPAD_FILE_PATH);
    if (!file) file = fs_create_file(NOTEPAD_FILE_PATH);
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
    const char* subtitle = "Starting Windows 7 style shell...";
    int title_w = (int)strlen(title) * 8;
    int subtitle_w = (int)strlen(subtitle) * 8;
    int bar_w = ((int)current_width * 2) / 3;
    int bar_h = 16;
    if (bar_w < 260) bar_w = 260;
    if (bar_w > 460) bar_w = 460;
    int bar_x = ((int)current_width - bar_w) / 2;
    int bar_y = ((int)current_height / 2) + 32;
    int title_x = ((int)current_width - title_w) / 2;
    int subtitle_x = ((int)current_width - subtitle_w) / 2;
    int title_y = ((int)current_height / 2) - 34;
    int subtitle_y = title_y + 18;

    int bg_h = (int)current_height;
    int band = bg_h / 3;
    int fill_target = bar_w - 8;
    int frames = 68;

    for (int f = 0; f <= frames; f++) {
        int eased_num = (2 * f * frames) - (f * f);
        int eased_den = frames * frames;
        int draw_w = (fill_target * eased_num) / eased_den;
        if (draw_w < 0) draw_w = 0;
        if (draw_w > fill_target) draw_w = fill_target;

        // Aero-like background bands.
        fill_rect(0, 0, current_width, band, XP_LBLUE);
        fill_rect(0, band, current_width, band, XP_BLUE);
        fill_rect(0, band * 2, current_width, bg_h - (band * 2), XP_CYAN);

        // Soft center panel.
        int panel_w = bar_w + 80;
        int panel_h = 96;
        int panel_x = ((int)current_width - panel_w) / 2;
        int panel_y = title_y - 12;
        fill_rect(panel_x, panel_y, panel_w, panel_h, XP_DGRAY);
        draw_rect(panel_x, panel_y, panel_w, panel_h, XP_LBLUE);
        draw_rect(panel_x + 1, panel_y + 1, panel_w - 2, panel_h - 2, XP_BLUE);

        draw_string(title_x, title_y, title, XP_WHITE);
        draw_string(subtitle_x, subtitle_y, subtitle, XP_LGRAY);

        // Progress rail.
        fill_rect(bar_x, bar_y, bar_w, bar_h, XP_BLACK);
        draw_rect(bar_x, bar_y, bar_w, bar_h, XP_WHITE);
        draw_rect(bar_x + 1, bar_y + 1, bar_w - 2, bar_h - 2, XP_DGRAY);
        fill_rect(bar_x + 4, bar_y + 4, draw_w, bar_h - 8, XP_LBLUE);

        // Moving highlight for modern feel.
        int glow = (f * 6) % (fill_target + 1);
        if (glow < draw_w) {
            fill_rect(bar_x + 4 + glow, bar_y + 4, 10, bar_h - 8, XP_WHITE);
        }

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
    fill_rect(x, y, w, h, XP_DGRAY);
    fill_rect(x + 2, y + 20, w - 4, h - 22, XP_WHITE);
    fill_rect(x, y, w, 18, is_active_window ? XP_BLUE : XP_DGRAY);
    fill_rect(x, y + 1, w, 4, is_active_window ? XP_LBLUE : XP_LGRAY);
    fill_rect(x, y + 14, w, 2, is_active_window ? XP_LBLUE : XP_DGRAY);
    draw_string(x + 6, y + 5, win->title, XP_WHITE);
    
    // Close button
    fill_rect(x + w - 16, y + 2, 14, 14, XP_RED);
    draw_rect(x + w - 16, y + 2, 14, 14, XP_WHITE);
    draw_rect(x + w - 15, y + 3, 12, 12, XP_DGRAY);
    draw_string(x + w - 12, y + 5, "x", XP_WHITE);
    
    draw_rect(x, y, w, h, XP_BLACK);
    draw_rect(x + 1, y + 1, w - 2, h - 2, XP_LBLUE);
    draw_rect(x + 2, y + 2, w - 4, h - 4, XP_DGRAY);
    // Bottom-right resize grip.
    for (int g = 0; g < 4; g++) {
        int gx = x + w - 3 - (g * 3);
        int gy = y + h - 2;
        draw_line(gx, gy, x + w - 2, y + h - 3 - (g * 3), XP_DGRAY);
    }
    
    if (win->type == WIN_NOTEPAD) {
        fill_rect(x + 4, y + 22, w - 8, h - 26, XP_LGRAY);
        draw_rect(x + 4, y + 22, w - 8, h - 26, XP_WHITE);
        draw_rect(x + 5, y + 23, w - 10, h - 28, XP_DGRAY);
        fill_rect(x + 6, y + 24, w - 12, 10, XP_BLUE);
        draw_compact_string(x + 8, y + 24, notepad_ui_toolbar_hint(), XP_BLACK);
        fill_rect(x + 5, y + h - 14, w - 10, 8, XP_BLUE);
        int text_top = y + 36;
        int text_bottom = y + h - 18;
        int visible_lines = (text_bottom - text_top) / 5;
        if (visible_lines < 1) visible_lines = 1;
        if (notepad_view_top < 0) notepad_view_top = 0;
        if (notepad_view_top > NOTEPAD_MAX_LINES - visible_lines) {
            notepad_view_top = NOTEPAD_MAX_LINES - visible_lines;
            if (notepad_view_top < 0) notepad_view_top = 0;
        }
        fill_rect(x + 8, y + 36, w - 16, h - 56, XP_WHITE);
        draw_rect(x + 8, y + 36, w - 16, h - 56, XP_LGRAY);
        int max_chars = (w - 22) / 6;
        if (max_chars < 1) max_chars = 1;
        if (max_chars > NOTEPAD_MAX_COLS) max_chars = NOTEPAD_MAX_COLS;
        for (int row = 0; row < visible_lines; row++) {
            int i = notepad_view_top + row;
            if (i >= NOTEPAD_MAX_LINES) break;
            if (notepad_lines[i][0]) {
                draw_compact_string_clipped(x + 11, text_top + row * 5, max_chars, notepad_lines[i], XP_BLACK);
            }
        }
        draw_compact_string(x + 8, y + h - 13, notepad_dirty ? notepad_ui_status_modified() : notepad_ui_status_saved(), XP_WHITE);
        if (win == &windows[active_window]) {
            int cx = x + 11 + notepad_cursor_x * 6;
            int cy = text_top + (notepad_cursor_y - notepad_view_top) * 5;
            if (cy >= text_top && cy < text_bottom) {
                fill_rect(cx, cy, 5, 6, XP_BLACK);
            }
        }
    } else if (win->type == WIN_SETTINGS) {
        fill_rect(x + 4, y + 22, w - 8, h - 26, XP_LGRAY);
        draw_rect(x + 4, y + 22, w - 8, h - 26, XP_WHITE);
        draw_rect(x + 5, y + 23, w - 10, h - 28, XP_DGRAY);
        int nav_x = x + 6;
        int nav_y = y + 24;
        int nav_w = 108;
        int nav_h = h - 30;
        fill_rect(nav_x, nav_y, nav_w, nav_h, XP_WHITE);
        draw_rect(nav_x, nav_y, nav_w, nav_h, XP_DGRAY);
        fill_rect(nav_x + 1, nav_y + 1, nav_w - 2, 12, XP_BLUE);
        draw_compact_string(nav_x + 6, nav_y + 3, settings_ui_panel_title(), XP_WHITE);

        int content_x = nav_x + nav_w + 6;
        int content_y = y + 24;
        int content_w = w - (content_x - x) - 6;
        int content_h = h - 30;
        int body_y = content_y + 12;
        fill_rect(content_x, content_y, content_w, content_h, XP_WHITE);
        draw_rect(content_x, content_y, content_w, content_h, XP_DGRAY);
        fill_rect(content_x + 1, content_y + 1, content_w - 2, 10, XP_LBLUE);
        draw_compact_string_clipped(content_x + 6, content_y + 2, (content_w - 12) / 6, "Modern Control Center", XP_WHITE);
        int content_chars = (content_w - 12) / 6;
        if (content_chars < 1) content_chars = 1;

        for (int i = 0; i < SETTINGS_TAB_COUNT; i++) {
            int ty = nav_y + 20 + i * 16;
            fill_rect(nav_x + 3, ty - 1, nav_w - 6, 13, (settings_tab == i) ? XP_LBLUE : XP_LGRAY);
            draw_rect(nav_x + 3, ty - 1, nav_w - 6, 13, XP_DGRAY);
            if (settings_tab == i) {
                draw_rect(nav_x + 4, ty, nav_w - 8, 11, XP_WHITE);
            }
            draw_compact_string_clipped(nav_x + 8, ty + 2, (nav_w - 14) / 6, settings_ui_tab_name(i), XP_BLACK);
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
            draw_compact_string_clipped(content_x + 6, body_y + 6, content_chars, "System Controls", XP_BLACK);
            draw_compact_string_clipped(content_x + 6, body_y + 22, content_chars, opt1, XP_BLACK);
            draw_compact_string_clipped(content_x + 6, body_y + 34, content_chars, opt2, XP_BLACK);
            draw_compact_string_clipped(content_x + 6, body_y + 46, content_chars, opt3, XP_BLACK);
            draw_compact_string_clipped(content_x + 6, body_y + 60, content_chars, "Storage devices:", XP_BLACK);
            int sc = fs_storage_get_device_count();
            char storage_line[44];
            strncpy(storage_line, "Detected profiles: ", sizeof(storage_line) - 1);
            storage_line[sizeof(storage_line) - 1] = 0;
            if (sc > 9) sc = 9;
            char c[2];
            c[0] = (char)('0' + sc);
            c[1] = 0;
            append_string(storage_line, sizeof(storage_line), c);
            draw_compact_string_clipped(content_x + 6, body_y + 70, content_chars, storage_line, XP_BLACK);
            draw_compact_string_clipped(content_x + 6, body_y + 78, content_chars, "HDD SSD NVMe USB CDROM RAM", XP_BLACK);
        } else if (settings_tab == SETTINGS_TAB_PERSONALIZATION) {
            char res_line[40];
            char scale_line[40];
            sprintf(res_line, "Desktop: %ux%u", (unsigned int)current_width, (unsigned int)current_height);
            sprintf(scale_line, "UI Scale Profile: x%d", get_ui_scale());
            draw_compact_string_clipped(content_x + 6, body_y + 6, content_chars, "Personalization", XP_BLACK);
            draw_compact_string_clipped(content_x + 6, body_y + 22, content_chars, "Theme: XP Classic", XP_BLACK);
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
            draw_compact_string_clipped(content_x + 6, body_y + 72, content_chars, "App root: C:/GamerOS/Apps", XP_BLACK);
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
            draw_compact_string_clipped(content_x + 6, body_y + 6, content_chars, "Gaming", XP_BLACK);
            draw_compact_string_clipped(content_x + 6, body_y + 22, content_chars, "Graphics mode: VGA 16-color", XP_BLACK);
            draw_compact_string_clipped(content_x + 6, body_y + 34, content_chars, "Input latency mode: Balanced", XP_BLACK);
        } else if (settings_tab == SETTINGS_TAB_ACCESSIBILITY) {
            draw_compact_string_clipped(content_x + 6, body_y + 6, content_chars, "Accessibility", XP_BLACK);
            draw_compact_string_clipped(content_x + 6, body_y + 22, content_chars, "High contrast: Off", XP_BLACK);
            draw_compact_string_clipped(content_x + 6, body_y + 34, content_chars, "Large text mode: Off", XP_BLACK);
        } else if (settings_tab == SETTINGS_TAB_PRIVACY_SECURITY) {
            draw_compact_string_clipped(content_x + 6, body_y + 6, content_chars, "Privacy & security", XP_BLACK);
            draw_compact_string_clipped(content_x + 6, body_y + 22, content_chars, "Auth model: Enabled", XP_BLACK);
            draw_compact_string_clipped(content_x + 6, body_y + 34, content_chars, "Isolation table: Active", XP_BLACK);
        } else if (settings_tab == SETTINGS_TAB_ABOUT) {
            char gfx_line[48];
            sprintf(gfx_line, "Graphics: VGA %ux%ux16", (unsigned int)current_width, (unsigned int)current_height);
            draw_compact_string_clipped(content_x + 6, body_y + 6, content_chars, "About GamerOS", XP_BLACK);
            draw_compact_string_clipped(content_x + 6, body_y + 22, content_chars, "Version: 00m1", XP_BLACK);
            draw_compact_string_clipped(content_x + 6, body_y + 34, content_chars, "Build: 1.300", XP_BLACK);
            draw_compact_string_clipped(content_x + 6, body_y + 46, content_chars, "Kernel: x86_64", XP_BLACK);
            draw_compact_string_clipped(content_x + 6, body_y + 58, content_chars, gfx_line, XP_BLACK);
            draw_compact_string_clipped(content_x + 6, body_y + 70, content_chars, "System32: C:/GamerOS/System32", XP_BLACK);
            draw_compact_string_clipped(content_x + 6, body_y + 82, content_chars, "Apps: *.EXE (NOTEPAD/SETTINGS/EXPLORER)", XP_BLACK);
            draw_compact_string_clipped(content_x + 6, body_y + 94, content_chars, "Author: Chosentechies", XP_BLACK);
        } else if (settings_tab == SETTINGS_TAB_GAMEROS_UPDATE) {
            settings_md_ensure_loaded();
            int total = settings_md_line_count;
            int row_h = 10;
            int visible = (content_h - 30) / row_h;
            if (visible < 1) visible = 1;
            int top = settings_scroll_top[SETTINGS_TAB_GAMEROS_UPDATE];
            if (top < 0) top = 0;
            if (top > total - visible) top = total - visible;
            if (top < 0) top = 0;
            settings_scroll_top[SETTINGS_TAB_GAMEROS_UPDATE] = top;
            for (int r = 0; r < visible; r++) {
                int idx = top + r;
                if (idx >= total) break;
                markdown_line_t* md = &settings_md_lines[idx];
                draw_compact_string_clipped(content_x + 6 + md->indent, body_y + 6 + r * row_h, content_chars, md->text, md->color);
            }
            draw_compact_string_clipped(content_x + 6, content_y + content_h - 10, content_chars, "Markdown viewer: mouse wheel scroll", XP_DGRAY);
        }
    } else if (win->type == WIN_EXPLORER) {
        fill_rect(x + 4, y + 22, w - 8, h - 26, XP_LGRAY);
        draw_rect(x + 4, y + 22, w - 8, h - 26, XP_WHITE);
        draw_rect(x + 5, y + 23, w - 10, h - 28, XP_DGRAY);

        fill_rect(x + 6, y + 24, w - 12, 12, XP_LBLUE);
        draw_compact_string(x + 10, y + 26, explorer_this_pc_view ? explorer_ui_title() : explorer_path, XP_WHITE);
        if (!explorer_this_pc_view) {
            fill_rect(x + w - 48, y + 25, 40, 10, XP_LGRAY);
            draw_rect(x + w - 48, y + 25, 40, 10, XP_DGRAY);
            draw_compact_string(x + w - 43, y + 26, "[Up]", XP_BLACK);
        }
        fill_rect(x + 8, y + 40, 82, h - 50, XP_WHITE);
        draw_rect(x + 8, y + 40, 82, h - 50, XP_DGRAY);
        draw_compact_string(x + 12, y + 44, "Quick Access", XP_BLACK);
        draw_compact_string(x + 12, y + 56, "Desktop", XP_BLACK);
        draw_compact_string(x + 12, y + 68, "Documents", XP_BLACK);
        draw_compact_string(x + 12, y + 80, "Downloads", XP_BLACK);
        draw_compact_string(x + 12, y + 92, "This PC", XP_BLACK);
        fill_rect(x + 94, y + 40, w - 100, h - 50, XP_WHITE);
        draw_rect(x + 94, y + 40, w - 100, h - 50, XP_DGRAY);
        int list_x = x + 100;
        int list_y = y + 44;
        int list_w = w - 110;
        int max_chars = list_w / 6;
        if (max_chars < 1) max_chars = 1;
        int row_h = 12;

        if (explorer_this_pc_view) {
            for (int i = 0; i < explorer_drive_count; i++) {
                int row_y = list_y + i * row_h;
                fill_rect(x + 96, row_y - 1, w - 104, row_h - 1, (explorer_selected == i) ? XP_LBLUE : XP_WHITE);
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
                fill_rect(x + 96, row_y - 1, w - 104, row_h - 1, (explorer_selected == i) ? XP_LBLUE : XP_WHITE);
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

    // Aero-like shell
    fill_rect(menu_x, menu_y, menu_w, menu_h, XP_DGRAY);
    draw_rect(menu_x, menu_y, menu_w, menu_h, XP_LBLUE);
    draw_rect(menu_x + 1, menu_y + 1, menu_w - 2, menu_h - 2, XP_BLUE);

    // Left profile pane
    fill_rect(menu_x + 4, menu_y + 4, 42, menu_h - 8, XP_BLUE);
    fill_rect(menu_x + 4, menu_y + 4, 42, 18, XP_LBLUE);
    draw_string(menu_x + 10, menu_y + 10, "GO", XP_WHITE);

    // Header bar
    fill_rect(menu_x + 48, menu_y + 4, menu_w - 52, 18, XP_BLUE);
    fill_rect(menu_x + 48, menu_y + 4, menu_w - 52, 4, XP_LBLUE);
    draw_string(menu_x + 52, menu_y + 9, "GamerOS", XP_WHITE);

    // Menu items
    const char* items[] = {
        "Notepad",
        "Settings",
        "File Explorer",
        "GamerOS Update",
        "About GamerOS"
    };
    int item_count = 5;
    for (int i = 0; i < item_count; i++) {
        int iy = menu_y + 28 + i * item_h;
        fill_rect(menu_x + 50, iy, menu_w - 56, item_h - 2, (i == 0) ? XP_LBLUE : XP_LGRAY);
        draw_rect(menu_x + 50, iy, menu_w - 56, item_h - 2, XP_DGRAY);
        draw_string(menu_x + 56, iy + 6, items[i], XP_BLACK);
    }

    fill_rect(menu_x + 48, menu_y + menu_h - 24, menu_w - 52, 16, XP_BLUE);
    draw_rect(menu_x + 48, menu_y + menu_h - 24, menu_w - 52, 16, XP_WHITE);
    draw_string(menu_x + 54, menu_y + menu_h - 20, "Shut Down", XP_WHITE);
}

void draw_desktop(void) {
    layout_desktop_icons();
    int desktop_h = (int)current_height - TASKBAR_HEIGHT;
    draw_desktop_wallpaper(desktop_h);
    if (setting_desktop_glow) {
        fill_rect(0, current_height - TASKBAR_HEIGHT - 20, current_width, 20, XP_LBLUE);
        fill_rect(0, current_height - TASKBAR_HEIGHT - 10, current_width, 10, XP_BLUE);
    }
    draw_desktop_watermark();
    
    for (int i = 0; i < (int)NUM_ICONS; i++) {
        int x = desktop_icons[i].x;
        int y = desktop_icons[i].y;
        draw_desktop_icon(x, y, desktop_icons[i].window_type);
        int label_w = (int)strlen(desktop_icons[i].label) * 6;
        int label_x = x + (DESKTOP_ICON_W / 2) - (label_w / 2);
        draw_compact_string(label_x, y + DESKTOP_ICON_H + 3, desktop_icons[i].label, XP_WHITE);
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
    fill_rect(0, taskbar_y, current_width, 3, XP_LBLUE);
    fill_rect(0, taskbar_y + 3, current_width, TASKBAR_HEIGHT - 3, XP_DGRAY);
    draw_rect(0, taskbar_y, current_width, TASKBAR_HEIGHT, XP_BLUE);

    // Start button (center text inside the button using compact font for consistent metrics)
    fill_rect(START_BTN_X, start_y, START_BTN_W, START_BTN_H, XP_BLUE);
    fill_rect(START_BTN_X, start_y, START_BTN_W, 4, XP_LBLUE);
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
    fill_rect(clock_x, clock_y, clock_w, clock_h, XP_LBLUE);
    draw_rect(clock_x, clock_y, clock_w, clock_h, XP_WHITE);
    {
        int clock_text_w = ((int)strlen(datetime_str)) * 6;
        int clock_text_x = clock_x + (clock_w - clock_text_w) / 2;
        int clock_text_y = clock_y + (clock_h - 8) / 2;
        draw_compact_string(clock_text_x, clock_text_y, datetime_str, XP_WHITE);
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
        int body_y = content_y + 12;
        int rel_x = mx - (content_x + 6);
        int rel_y = my - (body_y + 22);
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
    int nav_w = 108;
    int content_x = nav_x + nav_w + 6;
    int content_y = win->y + 24;
    int content_w = win->w - (content_x - win->x) - 6;
    int content_h = win->h - 30;
    if (mx < content_x || mx >= content_x + content_w || my < content_y || my >= content_y + content_h) {
        return 0;
    }

    if (settings_tab == SETTINGS_TAB_GAMEROS_UPDATE) {
        settings_md_ensure_loaded();
        int total = settings_md_line_count;
        int visible = (content_h - 30) / 10;
        if (visible < 1) visible = 1;
        int top = settings_scroll_top[SETTINGS_TAB_GAMEROS_UPDATE] - (int)wheel_delta;
        if (top < 0) top = 0;
        if (top > total - visible) top = total - visible;
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
            start_menu_metrics_t sm = {0};
            get_start_menu_metrics(&sm);
            int menu_x = sm.x;
            int menu_w = sm.w;
            int menu_h = sm.h;
            int menu_y = sm.y;
            int menu_item_h = sm.item_h;
            if (mx >= menu_x && mx < menu_x + menu_w && my >= menu_y && my < menu_y + menu_h) {
                int rel_y = my - (menu_y + 28);
                if (rel_y >= 0 && rel_y < menu_item_h) {
                    launch_application_exe("NOTEPAD.EXE", -1, -1);
                } else if (rel_y >= menu_item_h && rel_y < (menu_item_h * 2)) {
                    launch_application_exe("SETTINGS.EXE", -1, -1);
                } else if (rel_y >= (menu_item_h * 2) && rel_y < (menu_item_h * 3)) {
                    launch_application_exe("EXPLORER.EXE", -1, -1);
                } else if (rel_y >= (menu_item_h * 3) && rel_y < (menu_item_h * 4)) {
                    launch_settings_tab(SETTINGS_TAB_GAMEROS_UPDATE);
                } else if (rel_y >= (menu_item_h * 4) && rel_y < (menu_item_h * 5)) {
                    launch_settings_tab(SETTINGS_TAB_ABOUT);
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
