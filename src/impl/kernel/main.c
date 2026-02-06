#include "../../intf/graphics.h"
#include "../../intf/mouse.h"
#include "../../intf/keyboard.h"
#include "../../intf/serial.h"
#include "../../intf/io.h"
#include "stdint.h"
#include "string.h"

// Window states
#define MAX_WINDOWS 4
#define WIN_NONE    0
#define WIN_NOTEPAD 1
#define WIN_MYCOMP  2

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

// My Computer icons
#define ICON_COUNT 3
static const char* icon_names[ICON_COUNT] = {"A:", "C:", "CD"};

// Input state
static uint8_t last_buttons = 0;

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
                windows[i].w = 220;
                windows[i].h = 160;
            } else if (type == WIN_MYCOMP) {
                strncpy(windows[i].title, "My Computer", 31);
                windows[i].w = 180;
                windows[i].h = 140;
            }
            windows[i].title[31] = 0;
            
            active_window = i;
            window_count++;
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
    
    fill_rect(x, y, w, h, XP_WHITE);
    fill_rect(x, y, w, 18, (win == &windows[active_window]) ? XP_LBLUE : XP_DGRAY);
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
    } else if (win->type == WIN_MYCOMP) {
        for (int i = 0; i < ICON_COUNT; i++) {
            int ix = x + 20 + (i % 3) * 50;
            int iy = y + 40 + (i / 3) * 50;
            fill_rect(ix, iy, 32, 32, XP_LBLUE);
            draw_rect(ix, iy, 32, 32, XP_BLACK);
            draw_string(ix, iy + 36, icon_names[i], XP_BLACK);
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
    {20, 20, "Notepad", WIN_NOTEPAD},
    {20, 70, "My PC", WIN_MYCOMP},
};
#define NUM_ICONS (sizeof(desktop_icons) / sizeof(desktop_icons[0]))

void draw_desktop(void) {
    fill_rect(0, 0, current_width, current_height - 28, 0x39);
    
    for (int i = 0; i < (int)NUM_ICONS; i++) {
        int x = desktop_icons[i].x;
        int y = desktop_icons[i].y;
        fill_rect(x, y, 48, 40, XP_BLUE);
        draw_rect(x, y, 48, 40, XP_WHITE);
        draw_string(x + 4, y + 44, desktop_icons[i].label, XP_WHITE);
    }
    
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].active) {
            draw_window(&windows[i]);
        }
    }
    
    // Taskbar
    fill_rect(0, current_height - 28, current_width, 28, XP_BLUE);
    draw_rect(0, current_height - 28, current_width, 28, XP_WHITE);
    
    // Start button
    fill_rect(4, current_height - 24, 54, 20, XP_GREEN);
    draw_rect(4, current_height - 24, 54, 20, XP_WHITE);
    draw_string(10, current_height - 20, "Start", XP_WHITE);
    
    // Taskbar items
    int task_x = 64;
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].active) {
            if (i == active_window) {
                fill_rect(task_x, current_height - 24, 80, 20, XP_LBLUE);
            } else {
                fill_rect(task_x, current_height - 24, 80, 20, XP_BLUE);
            }
            draw_rect(task_x, current_height - 24, 80, 20, XP_WHITE);
            draw_string(task_x + 4, current_height - 20, windows[i].title, XP_WHITE);
            task_x += 84;
        }
    }
    
    // Clock
    draw_string(current_width - 40, current_height - 20, "12:00", XP_WHITE);
}

// Process mouse input
void process_mouse(int32_t mx, int32_t my, uint8_t buttons) {
    uint8_t pressed = buttons & ~last_buttons;
    
    if (pressed & MOUSE_BTN_LEFT) {
        // Check title bars for dragging
        for (int i = 0; i < MAX_WINDOWS; i++) {
            if (windows[i].active) {
                int tx = windows[i].x;
                int ty = windows[i].y;
                int tw = windows[i].w;
                
                if (mx >= tx && mx < tx + tw - 16 && my >= ty && my < ty + 18) {
                    windows[i].dragging = 1;
                    windows[i].drag_x = mx - windows[i].x;
                    windows[i].drag_y = my - windows[i].y;
                    active_window = i;
                    return;
                }
                
                // Check close button
                if (mx >= tx + tw - 16 && mx < tx + tw && my >= ty + 2 && my < ty + 16) {
                    close_window(i);
                    return;
                }
                
                if (mx >= tx && mx < tx + tw && my >= ty && my < ty + windows[i].h) {
                    active_window = i;
                }
            }
        }
        
        // Check desktop icons
        for (int i = 0; i < (int)NUM_ICONS; i++) {
            int ix = desktop_icons[i].x;
            int iy = desktop_icons[i].y;
            if (mx >= ix && mx < ix + 48 && my >= iy && my < iy + 48) {
                open_window(desktop_icons[i].type, 60 + i * 30, 40 + i * 20);
                return;
            }
        }
        
        // Check taskbar
        int task_x = 64;
        for (int i = 0; i < MAX_WINDOWS; i++) {
            if (windows[i].active) {
                if (mx >= task_x && mx < task_x + 80 && my >= (int)current_height - 24 && my < (int)current_height - 4) {
                    active_window = i;
                    return;
                }
                task_x += 84;
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
                if (windows[i].y > (int)current_height - 50) windows[i].y = current_height - 50;
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
void process_keyboard(void) {
    if (active_window < 0 || !windows[active_window].active) return;
    
    window_t* win = &windows[active_window];
    
    while (1) {
        char c = keyboard_getchar();
        if (!c) break;
        
        if (win->type == WIN_NOTEPAD) {
            if (c == '\b') {
                if (notepad_cursor_x > 0) {
                    notepad_cursor_x--;
                    notepad_lines[notepad_cursor_y][notepad_cursor_x] = 0;
                } else if (notepad_cursor_y > 0) {
                    notepad_cursor_y--;
                    notepad_cursor_x = 39;
                    while (notepad_cursor_x > 0 && !notepad_lines[notepad_cursor_y][notepad_cursor_x - 1]) {
                        notepad_cursor_x--;
                    }
                }
            } else if (c == '\n') {
                notepad_cursor_y++;
                notepad_cursor_x = 0;
                if (notepad_cursor_y >= NOTEPAD_MAX_LINES) notepad_cursor_y = NOTEPAD_MAX_LINES - 1;
            } else if (c >= 32 && c < 127) {
                notepad_lines[notepad_cursor_y][notepad_cursor_x] = c;
                notepad_cursor_x++;
                if (notepad_cursor_x >= NOTEPAD_MAX_COLS) {
                    notepad_cursor_x = 0;
                    notepad_cursor_y++;
                    if (notepad_cursor_y >= NOTEPAD_MAX_LINES) notepad_cursor_y = NOTEPAD_MAX_LINES - 1;
                }
            }
        }
    }
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
    
    // Main loop
    int32_t last_mx = -1, last_my = -1;
    
    while (1) {
        mouse_poll();
        keyboard_poll();
        
        int32_t mx = mouse_get_x();
        int32_t my = mouse_get_y();
        uint8_t buttons = mouse_get_buttons();
        
        process_mouse(mx, my, buttons);
        process_keyboard();
        
        if (mx != last_mx || my != last_my || buttons != last_buttons || keyboard_has_input()) {
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
