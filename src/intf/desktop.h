#ifndef DESKTOP_H
#define DESKTOP_H

// Desktop Management Functions
void desktop_init();
void desktop_update();
void desktop_handle_click();

// Window Management
int desktop_create_window(int x, int y, int width, int height, const char* title);
void desktop_close_window(int window_id);
void desktop_move_window(int window_id, int new_x, int new_y);

// Windows 11 specific functions
void w11_show_welcome();
void w11_draw_start_menu();
void w11_draw_system_tray();

#endif

