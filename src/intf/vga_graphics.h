#ifndef VGA_GRAPHICS_H
#define VGA_GRAPHICS_H

// VGA Graphics Functions
void vga_set_mode_13h();
void vga_clear_screen(unsigned char color);
void vga_put_pixel(int x, int y, unsigned char color);
void vga_draw_rectangle(int x, int y, int width, int height, unsigned char color);
void vga_draw_border(int x, int y, int width, int height, unsigned char color);
void vga_draw_char(int x, int y, char c, unsigned char color);
void vga_draw_string(int x, int y, const char* str, unsigned char color);

// Windows 11 Desktop Functions
void w11_draw_desktop();
void w11_draw_taskbar();
void w11_draw_window(int x, int y, int width, int height, const char* title);
void w11_draw_desktop_icons();

#endif

