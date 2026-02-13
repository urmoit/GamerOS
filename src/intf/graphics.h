#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "stdint.h"

// Video modes
#define VGA_MODE_13H_WIDTH 320
#define VGA_MODE_13H_HEIGHT 200
#define VGA_MODE_12H_WIDTH 640
#define VGA_MODE_12H_HEIGHT 480

// VESA Modes
#define VESA_MODE_640x480 0x101  // 640x480x256
#define VESA_MODE_800x600 0x103  // 800x600x256
#define VESA_MODE_1024x768 0x105 // 1024x768x256

typedef struct {
    uint32_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t framebuffer_bpp;
} vesa_info_t;

// Current mode info
extern uint32_t current_width;
extern uint32_t current_height;
extern uint8_t* framebuffer;
extern int graphics_mode;

// Mode types
typedef enum {
    MODE_VGA_320x200,
    MODE_VESA_640x480,
    MODE_VESA_800x600
} video_mode_t;

typedef video_mode_t vga_mode_t;

#define VGA_MODE_13H MODE_VGA_320x200
#define VGA_MODE_12H MODE_VESA_640x480

// Functions
void vga_set_mode_13h(void);
int vesa_set_mode(uint16_t mode);
void init_graphics(void);
void set_video_mode(video_mode_t mode);
int vga_set_mode(vga_mode_t mode);

// Drawing (uses back buffer)
void clear_screen(uint8_t color);
void draw_pixel(int x, int y, uint8_t color);
void fill_rect(int x, int y, int w, int h, uint8_t color);
void draw_rect(int x, int y, int w, int h, uint8_t color);
void draw_line(int x1, int y1, int x2, int y2, uint8_t color);
void swap_buffers(void);

// Text
void draw_char(int x, int y, char c, uint8_t color);
void draw_string(int x, int y, const char* str, uint8_t color);

// Compatibility wrappers for legacy callers
void vga_clear(uint8_t color);
void vga_set_pixel(int x, int y, uint8_t color);
uint8_t vga_get_pixel(int x, int y);
void vga_fill_rect(int x, int y, int w, int h, uint8_t color);
void vga_draw_rect(int x, int y, int w, int h, uint8_t color);
void vga_draw_line(int x1, int y1, int x2, int y2, uint8_t color);
void vga_draw_circle(int center_x, int center_y, int radius, uint8_t color);
void vga_fill_circle(int center_x, int center_y, int radius, uint8_t color);
void vga_draw_string(int x, int y, const char* str, uint8_t color);
void vga_blit_buffer(uint32_t* src_buffer, uint32_t src_width, uint32_t src_height,
                     int dest_x, int dest_y, int width, int height);
void vga_draw_bitmap_cursor(int x, int y);
void vga_draw_shadow(int x, int y, uint32_t w, uint32_t h, uint32_t color, uint8_t offset);
void vga_fill_rounded_rect(int x, int y, uint32_t w, uint32_t h, uint8_t radius, uint32_t color);
void vga_draw_rounded_rect(int x, int y, uint32_t w, uint32_t h, uint8_t radius, uint32_t color);

#define current_vga_width current_width
#define current_vga_height current_height
#define vga_framebuffer framebuffer
#define current_color_depth 8

// XP Colors
#define XP_BLACK      0x00
#define XP_BLUE       0x01
#define XP_GREEN      0x02
#define XP_CYAN       0x03
#define XP_RED        0x04
#define XP_MAGENTA    0x05
#define XP_BROWN      0x06
#define XP_LGRAY      0x07
#define XP_DGRAY      0x08
#define XP_LBLUE      0x09
#define XP_LGREEN     0x0A
#define XP_LCYAN      0x0B
#define XP_LRED       0x0C
#define XP_LMAGENTA   0x0D
#define XP_YELLOW     0x0E
#define XP_WHITE      0x0F
#define XP_COLOR_WHITE XP_WHITE
#define XP_DESKTOP    0x39
#define XP_TASKBAR    0x01

#endif
