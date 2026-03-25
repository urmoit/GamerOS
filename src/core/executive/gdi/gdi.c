#include "gdi.h"
#include <mm.h>
#include <string.h>

static int gdi_ready = 0;
static int gdi_last_error = GDI_ERR_NONE;

void gdi_init(void) {
    gdi_ready = 0;
    gdi_last_error = GDI_ERR_NONE;
    if (vga_set_mode(VGA_MODE_13H) != 1) {
        gdi_last_error = GDI_ERR_MODE_SET_FAILED;
        return;
    }
    gdi_ready = 1;
}

int gdi_is_ready(void) {
    return gdi_ready;
}

int gdi_get_last_error(void) {
    return gdi_last_error;
}

object_handle_t gdi_create_context(vga_mode_t mode) {
    if (!gdi_ready) {
        gdi_last_error = GDI_ERR_NOT_READY;
        return INVALID_HANDLE;
    }

    // Set the VGA mode
    if (vga_set_mode(mode) != 1) {
        gdi_last_error = GDI_ERR_MODE_SET_FAILED;
        return INVALID_HANDLE;
    }

    // Create context object
    gdi_context_t* context = (gdi_context_t*)kmalloc(sizeof(gdi_context_t));
    if (!context) {
        return INVALID_HANDLE;
    }

    context->mode = mode;
    context->width = current_vga_width;
    context->height = current_vga_height;
    context->color_depth = 8;
    context->framebuffer = vga_framebuffer;

    object_attributes_t attrs = {
        .type = OBJECT_TYPE_DEVICE,  // Use device type for graphics context
        .context = context
    };

    object_handle_t handle = object_create(OBJECT_TYPE_DEVICE, &attrs);
    if (handle == INVALID_HANDLE) {
        kfree(context);
        return INVALID_HANDLE;
    }

    context->handle = handle;
    return handle;
}

int gdi_destroy_context(object_handle_t context_handle) {
    if (object_get_type(context_handle) != OBJECT_TYPE_DEVICE) {
        return -1;
    }

    gdi_context_t* context = (gdi_context_t*)object_get_context(context_handle);
    if (context) {
        kfree(context);
    }

    return object_destroy(context_handle);
}

static gdi_context_t* get_context(object_handle_t handle) {
    if (!gdi_ready) return NULL;
    if (object_get_type(handle) != OBJECT_TYPE_DEVICE) {
        return NULL;
    }
    return (gdi_context_t*)object_get_context(handle);
}

int gdi_set_pixel(object_handle_t context_handle, uint32_t x, uint32_t y, uint32_t color) {
    gdi_context_t* context = get_context(context_handle);
    if (!context) return -1;

    vga_set_pixel(x, y, color);
    return 0;
}

uint32_t gdi_get_pixel(object_handle_t context_handle, uint32_t x, uint32_t y) {
    gdi_context_t* context = get_context(context_handle);
    if (!context) return 0;

    return vga_get_pixel(x, y);
}

int gdi_draw_line(object_handle_t context_handle, uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t color) {
    gdi_context_t* context = get_context(context_handle);
    if (!context) return -1;

    vga_draw_line(x1, y1, x2, y2, (uint8_t)color);
    return 0;
}

int gdi_draw_rect(object_handle_t context_handle, uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color) {
    gdi_context_t* context = get_context(context_handle);
    if (!context) return -1;

    vga_draw_rect(x, y, width, height, (uint8_t)color);
    return 0;
}

int gdi_fill_rect(object_handle_t context_handle, uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color) {
    gdi_context_t* context = get_context(context_handle);
    if (!context) return -1;

    vga_fill_rect(x, y, width, height, (uint8_t)color);
    return 0;
}

int gdi_draw_circle(object_handle_t context_handle, uint32_t center_x, uint32_t center_y, uint32_t radius, uint32_t color) {
    gdi_context_t* context = get_context(context_handle);
    if (!context) return -1;

    vga_draw_circle(center_x, center_y, radius, (uint8_t)color);
    return 0;
}

int gdi_fill_circle(object_handle_t context_handle, uint32_t center_x, uint32_t center_y, uint32_t radius, uint32_t color) {
    gdi_context_t* context = get_context(context_handle);
    if (!context) return -1;

    vga_fill_circle(center_x, center_y, radius, (uint8_t)color);
    return 0;
}

int gdi_draw_text(object_handle_t context_handle, uint32_t x, uint32_t y, const char* text, uint32_t color) {
    gdi_context_t* context = get_context(context_handle);
    if (!context || !text) return -1;

    vga_draw_string(x, y, text, (uint8_t)color);
    return 0;
}

int gdi_blit_buffer(object_handle_t context_handle, uint32_t* src_buffer, uint32_t src_width, uint32_t src_height,
                   uint32_t dest_x, uint32_t dest_y, uint32_t width, uint32_t height) {
    gdi_context_t* context = get_context(context_handle);
    if (!context || !src_buffer) return -1;

    vga_blit_buffer(src_buffer, src_width, src_height, dest_x, dest_y, width, height);
    return 0;
}

// TODO: Implement clipping regions for drawing operations
// TODO: Add support for alpha blending and transparency
// TODO: Implement double buffering for smoother graphics rendering
// TODO: Add support for hardware-accelerated graphics operations
