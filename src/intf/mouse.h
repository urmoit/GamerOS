#ifndef MOUSE_H
#define MOUSE_H

// Mouse Functions
void mouse_draw_cursor();
void mouse_erase_cursor();
void mouse_move(int delta_x, int delta_y);
void mouse_set_position(int x, int y);
void mouse_set_buttons(int buttons);
int mouse_get_x();
int mouse_get_y();
int mouse_get_buttons();
void mouse_show();
void mouse_hide();

// Windows 11 specific mouse functions
int mouse_is_over_start_button();
int mouse_is_over_window(int x, int y, int width, int height);

#endif

