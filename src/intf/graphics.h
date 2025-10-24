#ifndef GRAPHICS_H
#define GRAPHICS_H

void graphics_init();
void clear_screen(unsigned int color);
void put_pixel(int x, int y, unsigned int color);
void draw_rectangle(int x, int y, int width, int height, unsigned int color);
void draw_border(int x, int y, int width, int height, unsigned int color);
void draw_char(int x, int y, char c, unsigned int color);
void draw_string(int x, int y, const char* str, unsigned int color);

// Windows 11 Desktop Functions
void w11_draw_desktop();
void w11_draw_taskbar();
void w11_draw_window(int x, int y, int width, int height, const char* title);
void w11_draw_desktop_icons();

#endif
