#ifndef GDI_H
#define GDI_H

#include "../../intf/stdint.h"
#include "../../intf/graphics.h"  // Include existing graphics interfaces
#include "../object_manager/object_manager.h"

// GDI Context
typedef struct gdi_context {
    object_handle_t handle;
    vga_mode_t mode;
    uint32_t width;
    uint32_t height;
    uint32_t color_depth;
    uint8_t* framebuffer;
} gdi_context_t;

// Graphics Device Interface functions
object_handle_t gdi_create_context(vga_mode_t mode);
int gdi_destroy_context(object_handle_t context_handle);

int gdi_set_pixel(object_handle_t context_handle, uint32_t x, uint32_t y, uint32_t color);
uint32_t gdi_get_pixel(object_handle_t context_handle, uint32_t x, uint32_t y);

int gdi_draw_line(object_handle_t context_handle, uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t color);
int gdi_draw_rect(object_handle_t context_handle, uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color);
int gdi_fill_rect(object_handle_t context_handle, uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color);

int gdi_draw_circle(object_handle_t context_handle, uint32_t center_x, uint32_t center_y, uint32_t radius, uint32_t color);
int gdi_fill_circle(object_handle_t context_handle, uint32_t center_x, uint32_t center_y, uint32_t radius, uint32_t color);

int gdi_draw_text(object_handle_t context_handle, uint32_t x, uint32_t y, const char* text, uint32_t color);

// Advanced graphics
int gdi_blit_buffer(object_handle_t context_handle, uint32_t* src_buffer, uint32_t src_width, uint32_t src_height,
                   uint32_t dest_x, uint32_t dest_y, uint32_t width, uint32_t height);

// Initialization
void gdi_init(void);

// TODO: Add polygon drawing functions (draw_polygon, fill_polygon)
// TODO: Implement bitmap loading and rendering functions
// TODO: Add font management and text rendering with different fonts

#endif