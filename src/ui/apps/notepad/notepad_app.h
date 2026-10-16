#ifndef NOTEPAD_APP_H
#define NOTEPAD_APP_H

// Notepad standalone app interface.

#include <stdint.h>

#define NOTEPAD_MAX_LINES 160
#define NOTEPAD_MAX_COLS  96
#define NOTEPAD_FILE_PATH "GOS:/User/Notepad/NOTEPAD.TXT"

// Load persisted content on first use.
void notepad_app_init(void);

const char* notepad_app_title(void);

void notepad_app_draw_content(int x, int y, int w, int h, int is_active);

void notepad_app_draw_icon(int x, int y);

// Input. Each returns non-zero when it consumed the event.
int notepad_app_handle_click(int mx, int my);
int notepad_app_handle_wheel(int8_t wheel_delta);
int notepad_app_handle_key(char key);

int notepad_app_line_count(void);
int notepad_app_is_dirty(void);
void notepad_app_save(void);
void notepad_app_reload(void);

#endif
