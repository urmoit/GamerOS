#include <notepad_app.h>
#include <fs.h>
#include <graphics.h>
#include <keyboard.h>
#include <string.h>
#include <window.h>
#include <shell_api.h>

// All Notepad state is private to this module.
static char notepad_lines[NOTEPAD_MAX_LINES][NOTEPAD_MAX_COLS];
static int notepad_cursor_x = 0;
static int notepad_cursor_y = 0;
static int notepad_view_top = 0;
static int notepad_lines_shown = 1;
static uint8_t notepad_dirty = 0;
static uint8_t notepad_loaded = 0;
static uint8_t notepad_io_buffer[MAX_FILE_SIZE + 1];

// Save button rectangle, published by the renderer for the click handler.
static int notepad_save_x = 0, notepad_save_y = 0, notepad_save_w = 0, notepad_save_h = 0;

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
    file_t* file = shell_fs_open(NOTEPAD_FILE_PATH);
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
    file_t* file = shell_fs_open(NOTEPAD_FILE_PATH);
    if (!file) file = shell_fs_create(NOTEPAD_FILE_PATH);
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

void notepad_app_draw_content(int x, int y, int w, int h, int is_active) {
    const int TOOLBAR_H = 15;
    const int STATUS_H = 13;
    const int LINE_H = 8;
    // Wide enough for three-digit line numbers at 8px advance.
    const int GUTTER_W = 30;

    int tool_y = y + 28;
    int edit_y = tool_y + TOOLBAR_H + 3;
    int edit_h = h - (edit_y - y) - STATUS_H - 4;
    if (edit_h < LINE_H + 4) edit_h = LINE_H + 4;

    const int btn_w = 34;
    const int btn_h = 13;
    int bx = x + 6;
    draw_tool_button(bx, tool_y, btn_w, btn_h, "Save", 1);
    notepad_save_x = bx;
    notepad_save_y = tool_y;
    notepad_save_w = btn_w;
    notepad_save_h = btn_h;

    const char* state = notepad_dirty ? "Modified" : "Saved";
    int sx = x + w - 6 - ((int)strlen(state) * FONT_CW);
    draw_compact_string(sx, tool_y + (btn_h - 8) / 2, state, notepad_dirty ? XP_BLUE : XP_DGRAY);

    fill_chamfer_rect(x + 5, edit_y, w - 10, edit_h, XP_WHITE);
    draw_chamfer_rect(x + 5, edit_y, w - 10, edit_h, XP_DGRAY);

    int text_x = x + 5 + 2 + GUTTER_W + 2;
    int text_w = w - 10 - 4 - GUTTER_W - 2;
    if (text_w < 12) text_w = 12;
    int max_chars = text_w / FONT_CW;
    if (max_chars > NOTEPAD_MAX_COLS) max_chars = NOTEPAD_MAX_COLS;
    int max_chars_gutter = (GUTTER_W - 6) / FONT_CW;

    int lines_shown = (edit_h - 6) / LINE_H;
    if (lines_shown < 1) lines_shown = 1;
    notepad_lines_shown = lines_shown;

    if (notepad_view_top < 0) notepad_view_top = 0;
    int max_top = NOTEPAD_MAX_LINES - lines_shown;
    if (max_top < 0) max_top = 0;
    if (notepad_view_top > max_top) notepad_view_top = max_top;

    fill_rect(x + 5 + 2 + GUTTER_W, edit_y + 3, 1, edit_h - 6, XP_LGRAY);

    for (int row = 0; row < lines_shown; row++) {
        int i = notepad_view_top + row;
        if (i >= NOTEPAD_MAX_LINES) break;
        int row_y = edit_y + 3 + row * LINE_H;
        if (row_y + LINE_H > edit_y + edit_h - 1) break;

        char num[8];
        sprintf(num, "%d", i + 1);
        int nx = x + 5 + 2 + GUTTER_W - 2 - ((int)strlen(num) * FONT_CW);
        draw_compact_string_clipped(nx, row_y, max_chars_gutter, num, XP_DGRAY);

        if (notepad_lines[i][0]) {
            draw_compact_string_clipped(text_x, row_y, max_chars, notepad_lines[i], XP_BLACK);
        }
    }

    if (is_active) {
        int cx = text_x + notepad_cursor_x * FONT_CW;
        int cy = edit_y + 3 + (notepad_cursor_y - notepad_view_top) * LINE_H;
        if (cy >= edit_y + 3 && cy + LINE_H <= edit_y + edit_h - 1 && cx < x + w - 8) {
            fill_rect(cx, cy, 1, LINE_H - 1, XP_BLACK);
        }
    }

    int status_y = y + h - STATUS_H - 2;
    fill_chamfer_rect(x + 5, status_y, w - 10, STATUS_H, XP_LGRAY);
    draw_chamfer_rect(x + 5, status_y, w - 10, STATUS_H, XP_DGRAY);
    fill_rect(x + 6, status_y + 1, w - 12, 1, XP_WHITE);

    char pos[32];
    sprintf(pos, "Ln %d, Col %d", notepad_cursor_y + 1, notepad_cursor_x + 1);
    draw_compact_string_clipped(x + 9, status_y + 3, (w - 60) / FONT_CW, pos, XP_DGRAY);

    const char* fname = path_basename(NOTEPAD_FILE_PATH);
    int fw = (int)strlen(fname) * FONT_CW;
    if (fw < w - 90) {
        draw_compact_string_clipped(x + w - 8 - fw, status_y + 3, fw / FONT_CW + 1, fname, XP_DGRAY);
    }
}

void notepad_app_draw_icon(int x, int y) {
    fill_chamfer_rect(x, y, DESKTOP_ICON_W, DESKTOP_ICON_H, XP_WHITE);
    draw_chamfer_rect(x, y, DESKTOP_ICON_W, DESKTOP_ICON_H, XP_DGRAY);
    fill_rect(x + 2, y + 2, 3, DESKTOP_ICON_H - 4, XP_LBLUE);
    fill_rect(x + DESKTOP_ICON_W - 5, y, 5, 5, XP_LGRAY);
    draw_line(x + DESKTOP_ICON_W - 5, y + 4, x + DESKTOP_ICON_W - 1, y, XP_DGRAY);
    fill_rect(x + 7, y + 6, DESKTOP_ICON_W - 10, 1, XP_DGRAY);
    fill_rect(x + 7, y + 10, DESKTOP_ICON_W - 10, 1, XP_DGRAY);
    fill_rect(x + 7, y + 14, DESKTOP_ICON_W - 10, 1, XP_DGRAY);
}

int notepad_app_handle_wheel(int8_t wheel_delta) {
    if (wheel_delta == 0) return 0;
    int visible_lines = notepad_lines_shown;
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

int notepad_app_handle_click(int32_t mx, int32_t my) {
    
    if (notepad_save_h <= 0) return 0;
    if (mx < notepad_save_x || mx >= notepad_save_x + notepad_save_w) return 0;
    if (my < notepad_save_y || my >= notepad_save_y + notepad_save_h) return 0;
    shell_ensure_storage_initialized();
    if (shell_storage_ready()) notepad_save_to_storage();
    return 1;
}

const char* notepad_app_title(void) {
    return "Notepad";
}

        int notepad_app_handle_key(char c) {
    int consumed = 0;
            if ((uint8_t)c == KEY_PGUP) {
                shell_ensure_storage_initialized();
                if (shell_storage_ready()) notepad_save_to_storage();
                consumed = 1;
            } else if ((uint8_t)c == KEY_PGDN) {
                shell_ensure_storage_initialized();
                if (shell_storage_ready()) notepad_load_from_storage();
                consumed = 1;
            } else if ((uint8_t)c == KEY_DEL) {
                if (notepad_cursor_x < NOTEPAD_MAX_COLS - 1) {
                    for (int x = notepad_cursor_x; x < NOTEPAD_MAX_COLS - 1; x++) {
                        notepad_lines[notepad_cursor_y][x] = notepad_lines[notepad_cursor_y][x + 1];
                    }
                    notepad_lines[notepad_cursor_y][NOTEPAD_MAX_COLS - 1] = 0;
                    notepad_dirty = 1;
                    consumed = 1;
                }
            } else if (c == '\b') {
                if (notepad_cursor_x > 0) {
                    notepad_cursor_x--;
                    notepad_lines[notepad_cursor_y][notepad_cursor_x] = 0;
                    notepad_dirty = 1;
                    consumed = 1;
                } else if (notepad_cursor_y > 0) {
                    notepad_cursor_y--;
                    notepad_cursor_x = NOTEPAD_MAX_COLS - 1;
                    while (notepad_cursor_x > 0 && !notepad_lines[notepad_cursor_y][notepad_cursor_x - 1]) {
                        notepad_cursor_x--;
                    }
                    notepad_dirty = 1;
                    consumed = 1;
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
                consumed = 1;
            } else if ((uint8_t)c == KEY_RIGHT) {
                if (notepad_cursor_x < NOTEPAD_MAX_COLS - 1) {
                    notepad_cursor_x++;
                } else if (notepad_cursor_y < NOTEPAD_MAX_LINES - 1) {
                    notepad_cursor_y++;
                    notepad_cursor_x = 0;
                }
                consumed = 1;
            } else if ((uint8_t)c == KEY_UP) {
                if (notepad_cursor_y > 0) {
                    notepad_cursor_y--;
                    if (notepad_cursor_x >= NOTEPAD_MAX_COLS) {
                        notepad_cursor_x = NOTEPAD_MAX_COLS - 1;
                    }
                }
                consumed = 1;
            } else if ((uint8_t)c == KEY_DOWN) {
                if (notepad_cursor_y < NOTEPAD_MAX_LINES - 1) {
                    notepad_cursor_y++;
                    if (notepad_cursor_x >= NOTEPAD_MAX_COLS) {
                        notepad_cursor_x = NOTEPAD_MAX_COLS - 1;
                    }
                }
                consumed = 1;
            } else if ((uint8_t)c == KEY_HOME) {
                notepad_cursor_x = 0;
                consumed = 1;
            } else if ((uint8_t)c == KEY_END) {
                int end = 0;
                while (end < NOTEPAD_MAX_COLS && notepad_lines[notepad_cursor_y][end]) {
                    end++;
                }
                notepad_cursor_x = end;
                consumed = 1;
            } else if (c == '\n') {
                notepad_cursor_y++;
                notepad_cursor_x = 0;
                if (notepad_cursor_y >= NOTEPAD_MAX_LINES) notepad_cursor_y = NOTEPAD_MAX_LINES - 1;
                notepad_dirty = 1;
                consumed = 1;
            } else if (c >= 32 && c < 127) {
                notepad_lines[notepad_cursor_y][notepad_cursor_x] = c;
                notepad_cursor_x++;
                if (notepad_cursor_x >= NOTEPAD_MAX_COLS) {
                    notepad_cursor_x = 0;
                    notepad_cursor_y++;
                    if (notepad_cursor_y >= NOTEPAD_MAX_LINES) notepad_cursor_y = NOTEPAD_MAX_LINES - 1;
                }
                notepad_dirty = 1;
                consumed = 1;
            }

            // Keep the caret visible using the renderer-published row count.
            if (notepad_cursor_y < notepad_view_top) {
                notepad_view_top = notepad_cursor_y;
            } else {
                int visible_lines = notepad_lines_shown;
                if (visible_lines < 1) visible_lines = 1;
                if (notepad_cursor_y >= notepad_view_top + visible_lines) {
                    notepad_view_top = notepad_cursor_y - visible_lines + 1;
                }
            }
            return consumed;
        }
void notepad_app_init(void) {
    if (notepad_loaded) return;
    notepad_loaded = 1;
    notepad_clear();
    notepad_load_from_storage();
}

int notepad_app_line_count(void) { return NOTEPAD_MAX_LINES; }
int notepad_app_is_dirty(void) { return notepad_dirty; }

void notepad_app_save(void) {
    shell_ensure_storage_initialized();
    if (shell_storage_ready()) notepad_save_to_storage();
}

void notepad_app_reload(void) {
    shell_ensure_storage_initialized();
    if (shell_storage_ready()) notepad_load_from_storage();
}
