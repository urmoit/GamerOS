#include "../../intf/graphics.h"
#include "../../intf/mouse.h"
#include "../../intf/keyboard.h"
#include "../../intf/serial.h"
#include "../../intf/io.h"
#include "../../intf/ports.h"
#include "../../intf/rtc.h"
#include "../../intf/font.h"
#include "stdint.h"
#include "string.h"

// Window states
#define MAX_WINDOWS 4
#define WIN_NONE    0
#define WIN_NOTEPAD 1
#define WIN_MYCOMP  2
#define WIN_ABOUT   3

// Desktop shell sizing (compact XP-like layout for 320x200)
#define TASKBAR_HEIGHT         20
#define START_BTN_X            4
#define START_BTN_Y_PAD        3
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

typedef struct {
    int active;
    int type;
    int x, y;
    int w, h;
    int dragging;
    int drag_x, drag_y;
    char title[32];
} window_t;

static window_t windows[MAX_WINDOWS];
static int active_window = -1;
static int window_count = 0;

// Notepad content
#define NOTEPAD_MAX_LINES 20
#define NOTEPAD_MAX_COLS  40
static char notepad_lines[NOTEPAD_MAX_LINES][NOTEPAD_MAX_COLS];
static int notepad_cursor_x = 0;
static int notepad_cursor_y = 0;

// Input state
static uint8_t last_buttons = 0;
static uint8_t start_menu_open = 0;
static uint8_t shutdown_requested = 0;

static const char* about_lines[] = {
    "GamerOS About",
    "",
    "Version: 00m1",
    "Build: 1.100",
    "Kernel: x86_64",
    "Graphics: VGA 320x200x256",
    "",
    "Latest Changes:",
    "- VMware stability hardening",
    "- Start menu and taskbar refresh",
    "- Notepad drag/edit improvements",
    "- Font rendering fixes",
    "",
    "Author: Chosentechies"
};

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

static void startup_animation(void) {
    clear_screen(XP_BLUE);
    draw_string(98, 74, "GamerOS", XP_WHITE);
    draw_string(70, 88, "Professional", XP_WHITE);
    draw_rect(55, 112, 210, 10, XP_WHITE);
    for (int p = 0; p < 200; p++) {
        fill_rect(57, 114, p, 6, XP_LBLUE);
        swap_buffers();
        for (volatile int d = 0; d < 15000; d++) {
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
    for (int i = 0; i < NOTEPAD_MAX_LINES; i++) {
        for (int j = 0; j < NOTEPAD_MAX_COLS; j++) {
            notepad_lines[i][j] = 0;
        }
    }
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
            windows[i].w = 200;
            windows[i].h = 150;
            
            if (type == WIN_NOTEPAD) {
                strncpy(windows[i].title, "Notepad", 31);
                windows[i].w = 190;
                windows[i].h = 130;
            } else if (type == WIN_ABOUT) {
                strncpy(windows[i].title, "About GamerOS", 31);
                windows[i].w = 210;
                windows[i].h = 152;
            } else if (type == WIN_MYCOMP) {
                strncpy(windows[i].title, "My Computer", 31);
                windows[i].w = 180;
                windows[i].h = 140;
            }
            windows[i].title[31] = 0;
            window_count++;
            bring_window_to_front(i);
            return;
        }
    }
}

// Close window
void close_window(int idx) {
    if (idx >= 0 && idx < MAX_WINDOWS && windows[idx].active) {
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
    
    if (win->type == WIN_NOTEPAD) {
        fill_rect(x + 4, y + 22, w - 8, h - 26, XP_WHITE);
        draw_rect(x + 4, y + 22, w - 8, h - 26, XP_LGRAY);
        
        for (int i = 0; i < NOTEPAD_MAX_LINES && (y + 26 + i * 10) < y + h - 4; i++) {
            if (notepad_lines[i][0]) {
                draw_string(x + 8, y + 26 + i * 10, notepad_lines[i], XP_BLACK);
            }
        }
        
        if (win == &windows[active_window]) {
            int cx = x + 8 + notepad_cursor_x * 8;
            int cy = y + 26 + notepad_cursor_y * 10;
            fill_rect(cx, cy, 8, 10, XP_BLACK);
        }
    } else if (win->type == WIN_ABOUT) {
        fill_rect(x + 4, y + 22, w - 8, h - 26, XP_WHITE);
        draw_rect(x + 4, y + 22, w - 8, h - 26, XP_LGRAY);
        for (int i = 0; i < (int)(sizeof(about_lines) / sizeof(about_lines[0])); i++) {
            int yy = y + 26 + i * 8;
            if (yy > y + h - 10) break;
            draw_compact_string(x + 8, yy, about_lines[i], XP_BLACK);
        }
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
    {18, 62, "About", WIN_ABOUT},
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
    draw_string(menu_x + 52, menu_y + 34 + item_h, "About GamerOS", XP_BLACK);

    fill_rect(menu_x + 46, menu_y + 28 + (item_h * 2), menu_w - 52, item_h - 2, XP_LGRAY);
    draw_string(menu_x + 52, menu_y + 34 + (item_h * 2), "Run", XP_BLACK);

    fill_rect(menu_x + 44, menu_y + menu_h - 24, menu_w - 48, 16, XP_DGRAY);
    draw_string(menu_x + 50, menu_y + menu_h - 20, "Shut Down", XP_WHITE);
}

void draw_desktop(void) {
    fill_rect(0, 0, current_width, current_height - TASKBAR_HEIGHT, 0x39);
    fill_rect(0, current_height - TASKBAR_HEIGHT - 14, current_width, 14, XP_LBLUE);
    
    for (int i = 0; i < (int)NUM_ICONS; i++) {
        int x = desktop_icons[i].x;
        int y = desktop_icons[i].y;
        draw_notepad_icon(x, y);
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
    fill_rect(0, current_height - TASKBAR_HEIGHT, current_width, TASKBAR_HEIGHT, XP_BLUE);
    draw_rect(0, current_height - TASKBAR_HEIGHT, current_width, TASKBAR_HEIGHT, XP_WHITE);
    
    // Start button
    fill_rect(START_BTN_X, current_height - TASKBAR_HEIGHT + START_BTN_Y_PAD, START_BTN_W, START_BTN_H, XP_GREEN);
    draw_rect(START_BTN_X, current_height - TASKBAR_HEIGHT + START_BTN_Y_PAD, START_BTN_W, START_BTN_H, XP_WHITE);
    draw_string(START_BTN_X + 8, current_height - TASKBAR_HEIGHT + START_BTN_Y_PAD + 4, "Start", XP_WHITE);

    if (start_menu_open) {
        draw_start_menu();
    }
    
    // Taskbar items
    int task_x = 56;
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].active) {
            if (i == active_window) {
                fill_rect(task_x, current_height - TASKBAR_HEIGHT + 3, TASKBTN_W, TASKBTN_H, XP_LBLUE);
            } else {
                fill_rect(task_x, current_height - TASKBAR_HEIGHT + 3, TASKBTN_W, TASKBTN_H, XP_BLUE);
            }
            draw_rect(task_x, current_height - TASKBAR_HEIGHT + 3, TASKBTN_W, TASKBTN_H, XP_WHITE);

            char task_text[9];
            size_t len = strlen(windows[i].title);
            if (len > 8) len = 8;
            strncpy(task_text, windows[i].title, len);
            task_text[len] = 0;
            int tx = task_x + (TASKBTN_W - ((int)len * 6)) / 2;
            draw_compact_string(tx, current_height - TASKBAR_HEIGHT + 6, task_text, XP_WHITE);
            task_x += TASKBTN_W + 2;
        }
    }
    
    // Clock
    uint8_t hour = 0, minute = 0, second = 0;
    get_time(&hour, &minute, &second);
    char time_str[6];
    time_str[0] = (char)('0' + ((hour / 10) % 10));
    time_str[1] = (char)('0' + (hour % 10));
    time_str[2] = ':';
    time_str[3] = (char)('0' + ((minute / 10) % 10));
    time_str[4] = (char)('0' + (minute % 10));
    time_str[5] = 0;
    draw_string(current_width - 42, current_height - TASKBAR_HEIGHT + 7, time_str, XP_WHITE);
}

// Process mouse input
void process_mouse(int32_t mx, int32_t my, uint8_t buttons) {
    uint8_t pressed = buttons & ~last_buttons;
    
    if (pressed & MOUSE_BTN_LEFT) {
        // Start button
        int start_y = (int)current_height - TASKBAR_HEIGHT + START_BTN_Y_PAD;
        if (mx >= START_BTN_X && mx < START_BTN_X + START_BTN_W &&
            my >= start_y && my < start_y + START_BTN_H) {
            start_menu_open = !start_menu_open;
            last_buttons = buttons;
            return;
        }

        // Start menu items
        if (start_menu_open) {
            int menu_x = START_MENU_X;
            int menu_w = START_MENU_W;
            int menu_h = START_MENU_H;
            int menu_y = (int)current_height - TASKBAR_HEIGHT - menu_h;
            int menu_item_h = START_MENU_ITEM_H;
            if (mx >= menu_x && mx < menu_x + menu_w && my >= menu_y && my < menu_y + menu_h) {
                int rel_y = my - (menu_y + 24);
                if (rel_y >= 0 && rel_y < menu_item_h) {
                    open_window(WIN_NOTEPAD, 80, 50);
                } else if (rel_y >= menu_item_h && rel_y < (menu_item_h * 2)) {
                    open_window(WIN_ABOUT, 62, 26);
                } else if (my >= menu_y + menu_h - 24 && my < menu_y + menu_h - 8 &&
                           mx >= menu_x + 44 && mx < menu_x + menu_w - 4) {
                    shutdown_os();
                }
                start_menu_open = 0;
                last_buttons = buttons;
                return;
            }
            start_menu_open = 0;
        }

        // Check title bars for dragging
        int focus_idx = -1;
        for (int i = 0; i < MAX_WINDOWS; i++) {
            if (windows[i].active) {
                int tx = windows[i].x;
                int ty = windows[i].y;
                int tw = windows[i].w;
                
                if (mx >= tx && mx < tx + tw - 16 && my >= ty && my < ty + 18) {
                    windows[i].dragging = 1;
                    windows[i].drag_x = mx - windows[i].x;
                    windows[i].drag_y = my - windows[i].y;
                    bring_window_to_front(i);
                    return;
                }
                
                // Check close button
                if (mx >= tx + tw - 16 && mx < tx + tw && my >= ty + 2 && my < ty + 16) {
                    close_window(i);
                    return;
                }
                
                if (mx >= tx && mx < tx + tw && my >= ty && my < ty + windows[i].h) {
                    focus_idx = i;
                }
            }
        }

        if (focus_idx >= 0) {
            bring_window_to_front(focus_idx);
            return;
        }
        
        // Check desktop icons
        for (int i = 0; i < (int)NUM_ICONS; i++) {
            int ix = desktop_icons[i].x;
            int iy = desktop_icons[i].y;
            if (mx >= ix && mx < ix + DESKTOP_ICON_HIT_W &&
                my >= iy && my < iy + DESKTOP_ICON_HIT_H) {
                open_window(desktop_icons[i].type, 60 + i * 30, 40 + i * 20);
                return;
            }
        }
        
        // Check taskbar
        int task_x = 56;
        for (int i = 0; i < MAX_WINDOWS; i++) {
            if (windows[i].active) {
                if (mx >= task_x && mx < task_x + TASKBTN_W &&
                    my >= (int)current_height - TASKBAR_HEIGHT + 3 &&
                    my < (int)current_height - TASKBAR_HEIGHT + 17) {
                    bring_window_to_front(i);
                    return;
                }
                task_x += TASKBTN_W + 2;
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
            }
        }
    }
    
    // Stop dragging
    if ((~buttons & last_buttons) & MOUSE_BTN_LEFT) {
        for (int i = 0; i < MAX_WINDOWS; i++) {
            windows[i].dragging = 0;
        }
    }
    
    last_buttons = buttons;
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
            if (c == '\b') {
                if (notepad_cursor_x > 0) {
                    notepad_cursor_x--;
                    notepad_lines[notepad_cursor_y][notepad_cursor_x] = 0;
                    changed = 1;
                } else if (notepad_cursor_y > 0) {
                    notepad_cursor_y--;
                    notepad_cursor_x = 39;
                    while (notepad_cursor_x > 0 && !notepad_lines[notepad_cursor_y][notepad_cursor_x - 1]) {
                        notepad_cursor_x--;
                    }
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
                changed = 1;
            } else if (c >= 32 && c < 127) {
                notepad_lines[notepad_cursor_y][notepad_cursor_x] = c;
                notepad_cursor_x++;
                if (notepad_cursor_x >= NOTEPAD_MAX_COLS) {
                    notepad_cursor_x = 0;
                    notepad_cursor_y++;
                    if (notepad_cursor_y >= NOTEPAD_MAX_LINES) notepad_cursor_y = NOTEPAD_MAX_LINES - 1;
                }
                changed = 1;
            }
        }
    }
    return changed;
}

// Main kernel entry
void kernel_main(void) {
    serial_init();
    serial_write_string("GamerOS Starting...\n");
    
    // Set graphics mode
    set_video_mode(MODE_VGA_320x200);
    
    // Initialize subsystems
    keyboard_init();
    mouse_init();
    init_windows();
    startup_animation();

    // VMware stability mode: use polling-only input in the main loop.
    // IRQ-driven mouse/keyboard paths can still trigger fault storms while dragging.
    __asm__ volatile ("cli");
    
    // Main loop
    int32_t last_mx = -1, last_my = -1;
    
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
        uint8_t prev_buttons = last_buttons;
        
        process_mouse(mx, my, buttons);
        int keyboard_changed = process_keyboard();
        
        if (mx != last_mx || my != last_my || buttons != prev_buttons || keyboard_changed) {
            clear_screen(0);
            draw_desktop();
            draw_cursor(mx, my);
            swap_buffers();
            
            last_mx = mx;
            last_my = my;
            last_buttons = buttons;
        }
    }
}
