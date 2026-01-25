#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "stdint.h"

// Supported video modes
#define VGA_MODE_13H_WIDTH 320
#define VGA_MODE_13H_HEIGHT 200
#define VGA_MODE_12H_WIDTH 640
#define VGA_MODE_12H_HEIGHT 480
#define VGA_MODE_101H_WIDTH 640
#define VGA_MODE_101H_HEIGHT 480
#define VGA_MODE_103H_WIDTH 800
#define VGA_MODE_103H_HEIGHT 600
#define VGA_MODE_118H_WIDTH 1024
#define VGA_MODE_118H_HEIGHT 768

// Color depth definitions
#define COLOR_DEPTH_8BIT   8
#define COLOR_DEPTH_16BIT  16
#define COLOR_DEPTH_24BIT  24
#define COLOR_DEPTH_32BIT  32

// Default mode (can be changed at runtime)
#define VGA_WIDTH VGA_MODE_13H_WIDTH
#define VGA_HEIGHT VGA_MODE_13H_HEIGHT
#define VGA_MODE13_SEGMENT 0xA000

// Video mode enumeration
typedef enum {
    VGA_MODE_13H = 0x13,    // 320x200x256 (default)
    VGA_MODE_12H = 0x12,    // 640x480x16
    VGA_MODE_101H = 0x101,  // 640x480x256
    VGA_MODE_103H = 0x103,  // 800x600x256
    VGA_MODE_118H = 0x118   // 1024x768x24 (VESA)
} vga_mode_t;

// Color structures for different depths
typedef union {
    uint8_t components[4];  // RGBA
    uint32_t value;         // 32-bit packed
} color_32bit_t;

typedef union {
    uint8_t components[3];  // RGB
    uint16_t value;         // 24-bit packed (stored in 32-bit for alignment)
} color_24bit_t;

typedef uint16_t color_16bit_t;  // 5:6:5 RGB
typedef uint8_t color_8bit_t;    // 256 color palette

// Current video mode state
extern uint32_t current_vga_width;
extern uint32_t current_vga_height;
extern uint32_t current_color_depth;
extern vga_mode_t current_vga_mode;
extern int graphics_initialized;

// Initialize VGA graphics modes
void vga_init_mode13(void);
void vga_init_mode12h(void);
int vga_init_mode101h(void);
int vga_init_mode103h(void);
int vga_init_mode118h(void);
int vga_set_mode(vga_mode_t mode);

// Set a pixel at (x, y) with color (supports different color depths)
void vga_set_pixel(uint32_t x, uint32_t y, uint32_t color);

// Get pixel color at (x, y) (returns color in current depth format)
uint32_t vga_get_pixel(uint32_t x, uint32_t y);

// Color conversion utilities
uint32_t rgb_to_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
void color_to_rgb(uint32_t color, uint8_t* r, uint8_t* g, uint8_t* b, uint8_t* a);

// Fill a rectangle with color
void vga_fill_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint8_t color);
void vga_draw_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint8_t color);
void vga_set_desktop_background(void);
void vga_clear(uint8_t color);

// Advanced drawing primitives
void vga_draw_line(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint8_t color);
void vga_draw_circle(uint32_t center_x, uint32_t center_y, uint32_t radius, uint8_t color);
void vga_fill_circle(uint32_t center_x, uint32_t center_y, uint32_t radius, uint8_t color);
void vga_draw_triangle(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t x3, uint32_t y3, uint8_t color);
void vga_fill_triangle(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t x3, uint32_t y3, uint8_t color);

// Graphics utilities
void vga_draw_horizontal_line(uint32_t x, uint32_t y, uint32_t length, uint8_t color);
void vga_draw_vertical_line(uint32_t x, uint32_t y, uint32_t length, uint8_t color);

// Performance optimizations
void vga_fast_fill_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint8_t color);
void vga_fast_clear(uint8_t color);

// Text rendering (basic bitmap font)
void vga_draw_char(uint32_t x, uint32_t y, char c, uint8_t color);
void vga_draw_string(uint32_t x, uint32_t y, const char* str, uint8_t color);

// Software rendering pipeline
typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t* pixels;  // 32-bit RGBA buffer
} render_buffer_t;

// Rendering pipeline functions
render_buffer_t* create_render_buffer(uint32_t width, uint32_t height);
void destroy_render_buffer(render_buffer_t* buffer);
void clear_render_buffer(render_buffer_t* buffer, uint32_t color);
void render_buffer_to_screen(render_buffer_t* buffer, uint32_t screen_x, uint32_t screen_y);

// 2D rendering functions (software)
void draw_pixel_software(render_buffer_t* buffer, int32_t x, int32_t y, uint32_t color);
void draw_line_software(render_buffer_t* buffer, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color);
void draw_rect_software(render_buffer_t* buffer, int32_t x, int32_t y, uint32_t width, uint32_t height, uint32_t color);
void fill_rect_software(render_buffer_t* buffer, int32_t x, int32_t y, uint32_t width, uint32_t height, uint32_t color);
void draw_circle_software(render_buffer_t* buffer, int32_t center_x, int32_t center_y, uint32_t radius, uint32_t color);
void fill_circle_software(render_buffer_t* buffer, int32_t center_x, int32_t center_y, uint32_t radius, uint32_t color);

// Alpha blending
uint32_t blend_colors(uint32_t src, uint32_t dst);

// Graphics acceleration optimizations
void vga_blit_buffer(uint32_t* src_buffer, uint32_t src_width, uint32_t src_height,
                      uint32_t dest_x, uint32_t dest_y, uint32_t width, uint32_t height);
void vga_blit_buffer_scaled(uint32_t* src_buffer, uint32_t src_width, uint32_t src_height,
                            uint32_t dest_x, uint32_t dest_y, uint32_t dest_width, uint32_t dest_height);

// Fast memory operations
void vga_memcpy_fast(void* dest, const void* src, uint32_t count);
void vga_memset_fast(void* dest, uint32_t value, uint32_t count);

// Double buffering support
typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t* front_buffer;
    uint32_t* back_buffer;
    uint8_t current_buffer;
} double_buffer_t;

double_buffer_t* create_double_buffer(uint32_t width, uint32_t height);
void destroy_double_buffer(double_buffer_t* db);
void swap_buffers(double_buffer_t* db);
uint32_t* get_current_buffer(double_buffer_t* db);
void present_buffer(double_buffer_t* db);

// Advanced graphics primitives for UI framework

// Gradient fills
void vga_draw_gradient_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height,
                           uint32_t color_start, uint32_t color_end, uint8_t vertical);
void vga_draw_radial_gradient(uint32_t center_x, uint32_t center_y, uint32_t radius,
                             uint32_t color_center, uint32_t color_edge);

// Shadow and glow effects
void vga_draw_shadow(uint32_t x, uint32_t y, uint32_t width, uint32_t height,
                    uint32_t shadow_color, uint8_t blur_radius);
void vga_draw_glow(uint32_t x, uint32_t y, uint32_t width, uint32_t height,
                  uint32_t glow_color, uint8_t intensity);

// Rounded rectangles and advanced shapes
void vga_draw_rounded_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height,
                          uint32_t radius, uint8_t color);
void vga_fill_rounded_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height,
                          uint32_t radius, uint8_t color);

// Bezier curves for smooth shapes
void vga_draw_bezier_curve(uint32_t x1, uint32_t y1, uint32_t cx1, uint32_t cy1,
                          uint32_t cx2, uint32_t cy2, uint32_t x2, uint32_t y2,
                          uint8_t color, uint8_t thickness);

// Anti-aliased lines and circles
void vga_draw_aa_line(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t color);
void vga_draw_aa_circle(uint32_t center_x, uint32_t center_y, uint32_t radius, uint32_t color);

// Text effects
void vga_draw_text_shadow(uint32_t x, uint32_t y, const char* str, uint8_t text_color, uint8_t shadow_color);
void vga_draw_text_outline(uint32_t x, uint32_t y, const char* str, uint8_t text_color, uint8_t outline_color);

// Image and sprite support
typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t* pixels;  // RGBA pixels
} image_t;

image_t* create_image(uint32_t width, uint32_t height);
void destroy_image(image_t* img);
void draw_image(uint32_t x, uint32_t y, image_t* img);
void draw_image_scaled(uint32_t x, uint32_t y, uint32_t dest_width, uint32_t dest_height, image_t* img);

// Alpha blending for transparency
uint32_t blend_pixel(uint32_t src, uint32_t dst);

// Mouse cursor drawing
void vga_draw_mouse_cursor(int32_t x, int32_t y, uint8_t color);
void vga_draw_mouse_arrow(int32_t x, int32_t y, uint8_t color);
void vga_erase_mouse_area(int32_t x, int32_t y);

// TODO: Add support for 3D graphics primitives (cubes, spheres)
// TODO: Implement texture mapping and UV coordinates
// TODO: Add particle system for effects
// TODO: Implement GPU-accelerated rendering functions

#endif



