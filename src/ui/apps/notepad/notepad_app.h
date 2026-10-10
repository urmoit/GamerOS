#ifndef NOTEPAD_APP_H
#define NOTEPAD_APP_H

// Notepad, packaged as a standalone app rather than part of the kernel image.
// The shell (window manager, taskbar, input pump) lives in the kernel and calls
// into this module; this module owns all of Notepad's state and its own files.

#include <stdint.h>

#define NOTEPAD_MAX_LINES 160
#define NOTEPAD_MAX_COLS  96
#define NOTEPAD_FILE_PATH "GOS:/User/Notepad/NOTEPAD.TXT"

// Load persisted content on first use.
void notepad_app_init(void);

// Title shown in the window frame and taskbar.
const char* notepad_app_title(void);

// Draw the window body (toolbar, gutter, editor, status bar) into the given rect.
void notepad_app_draw_content(int x, int y, int w, int h, int is_active);

// Desktop icon.
void notepad_app_draw_icon(int x, int y);

// Input. Each returns non-zero when it consumed the event.
int notepad_app_handle_click(int mx, int my);
int notepad_app_handle_wheel(int8_t wheel_delta);
int notepad_app_handle_key(char key);

// Exposed for the About window and for tests.
int notepad_app_line_count(void);
int notepad_app_is_dirty(void);
void notepad_app_save(void);
void notepad_app_reload(void);

#endif
