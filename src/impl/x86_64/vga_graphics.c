#include "../../intf/graphics.h"
#include "../../intf/ui.h"

// Global video mode state
uint32_t current_vga_width = VGA_WIDTH;
uint32_t current_vga_height = VGA_HEIGHT;
uint32_t current_color_depth = COLOR_DEPTH_8BIT;
vga_mode_t current_vga_mode = VGA_MODE_13H;
int graphics_initialized = 0; // Track if graphics mode was successfully initialized

// VGA framebuffers for different modes
static uint8_t* vga_framebuffer_13h = (uint8_t*)0xA0000;  // Mode 13h: 320x200
static uint8_t* vga_framebuffer_12h = (uint8_t*)0xA0000;  // Mode 12h: 640x480 (planar)
static uint8_t* vga_framebuffer_101h = (uint8_t*)0xA0000; // Mode 101h: 640x480
static uint8_t* vga_framebuffer_103h = (uint8_t*)0xA0000; // Mode 103h: 800x600

// Current active framebuffer
static uint8_t* vga_framebuffer = (uint8_t*)0xA0000;

void vga_init_mode13(void) {
    // Switch to VGA mode 13h (320x200x256)
    __asm__ volatile (
        "mov $0x13, %%ah\n"
        "int $0x10\n"
        :
        :
        : "ah"
    );
    vga_framebuffer = vga_framebuffer_13h;
    current_vga_width = VGA_MODE_13H_WIDTH;
    current_vga_height = VGA_MODE_13H_HEIGHT;
    current_vga_mode = VGA_MODE_13H;
}

void vga_init_mode12h(void) {
    // Switch to VGA mode 12h (640x480x16)
    __asm__ volatile (
        "mov $0x12, %%ah\n"
        "int $0x10\n"
        :
        :
        : "ah"
    );
    vga_framebuffer = vga_framebuffer_12h;
    current_vga_width = VGA_MODE_12H_WIDTH;
    current_vga_height = VGA_MODE_12H_HEIGHT;
    current_vga_mode = VGA_MODE_12H;
}

int vga_init_mode101h(void) {
    // Check if VESA mode was already set up in real mode
    extern char vesa_success[];
    if (vesa_success[0]) {
        // VESA mode already set, just configure our variables
        vga_framebuffer = (uint8_t*)0xA0000; // Standard VGA location, but VESA LFB might be elsewhere
        current_vga_width = VGA_MODE_101H_WIDTH;
        current_vga_height = VGA_MODE_101H_HEIGHT;
        current_vga_mode = VGA_MODE_101H;
        return 1; // Success - already set
    }

    // Fallback: try BIOS call (may not work in long mode)
    uint16_t result;
    __asm__ volatile (
        "mov $0x4F02, %%ax\n"
        "mov $0x101, %%bx\n"
        "int $0x10\n"
        "mov %%ax, %0\n"
        : "=r"(result)
        :
        : "ax", "bx"
    );

    // Check if VESA call succeeded (should return 0x004F on success)
    if ((result & 0x00FF) != 0x4F) {
        return 0; // Failed
    }

    vga_framebuffer = vga_framebuffer_101h;
    current_vga_width = VGA_MODE_101H_WIDTH;
    current_vga_height = VGA_MODE_101H_HEIGHT;
    current_vga_mode = VGA_MODE_101H;
    return 1; // Success
}

int vga_init_mode103h(void) {
    // Switch to VESA mode 103h (800x600x256)
    uint16_t result;
    __asm__ volatile (
        "mov $0x4F02, %%ax\n"
        "mov $0x103, %%bx\n"
        "int $0x10\n"
        "mov %%ax, %0\n"
        : "=r"(result)
        :
        : "ax", "bx"
    );

    // Check if VESA call succeeded (should return 0x004F on success)
    if ((result & 0x00FF) != 0x4F) {
        return 0; // Failed
    }

    vga_framebuffer = vga_framebuffer_103h;
    current_vga_width = VGA_MODE_103H_WIDTH;
    current_vga_height = VGA_MODE_103H_HEIGHT;
    current_color_depth = COLOR_DEPTH_8BIT;
    current_vga_mode = VGA_MODE_103H;
    return 1; // Success
}

int vga_init_mode118h(void) {
    // Switch to VESA mode 118h (1024x768x24)
    uint16_t result;
    __asm__ volatile (
        "mov $0x4F02, %%ax\n"
        "mov $0x118, %%bx\n"
        "int $0x10\n"
        "mov %%ax, %0\n"
        : "=r"(result)
        :
        : "ax", "bx"
    );

    // Check if VESA call succeeded (should return 0x004F on success)
    if ((result & 0x00FF) != 0x4F) {
        return 0; // Failed
    }

    vga_framebuffer = vga_framebuffer_13h; // Will be set by VESA
    current_vga_width = VGA_MODE_118H_WIDTH;
    current_vga_height = VGA_MODE_118H_HEIGHT;
    current_color_depth = COLOR_DEPTH_24BIT;
    current_vga_mode = VGA_MODE_118H;
    return 1; // Success
}

int vga_set_mode(vga_mode_t mode) {
    int result = 0;
    switch (mode) {
        case VGA_MODE_13H:
            vga_init_mode13();
            result = 1; // Mode 13h always succeeds
            break;
        case VGA_MODE_12H:
            vga_init_mode12h();
            result = 1; // Mode 12h always succeeds
            break;
        case VGA_MODE_101H:
            result = vga_init_mode101h();
            break;
        case VGA_MODE_103H:
            result = vga_init_mode103h();
            break;
        case VGA_MODE_118H:
            result = vga_init_mode118h();
            break;
        default:
            result = 0; // Unknown mode
            break;
    }

    graphics_initialized = result;
    return result;
}

void vga_set_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (!graphics_initialized || x >= current_vga_width || y >= current_vga_height) {
        return;
    }

    uint32_t offset = y * current_vga_width + x;

    // Handle different color depths and video modes
    switch (current_color_depth) {
        case COLOR_DEPTH_8BIT:
            // 8-bit palette mode
            switch (current_vga_mode) {
                case VGA_MODE_13H:
                case VGA_MODE_101H:
                case VGA_MODE_103H:
                    ((uint8_t*)vga_framebuffer)[offset] = (uint8_t)color;
                    break;
                case VGA_MODE_12H:
                    // Planar mode - simplified (only 16 colors)
                    ((uint8_t*)vga_framebuffer)[offset] = (uint8_t)(color & 0x0F);
                    break;
            }
            break;

        case COLOR_DEPTH_16BIT:
            // 16-bit RGB (5:6:5)
            ((uint16_t*)vga_framebuffer)[offset] = (uint16_t)color;
            break;

        case COLOR_DEPTH_24BIT:
            // 24-bit RGB
            {
                uint8_t* pixel = (uint8_t*)vga_framebuffer + (offset * 3);
                pixel[0] = (color >> 16) & 0xFF; // Red
                pixel[1] = (color >> 8) & 0xFF;  // Green
                pixel[2] = color & 0xFF;         // Blue
            }
            break;

        case COLOR_DEPTH_32BIT:
            // 32-bit RGBA
            ((uint32_t*)vga_framebuffer)[offset] = color;
            break;
    }
}

uint32_t vga_get_pixel(uint32_t x, uint32_t y) {
    if (x >= current_vga_width || y >= current_vga_height) {
        return 0;
    }

    uint32_t offset = y * current_vga_width + x;

    switch (current_color_depth) {
        case COLOR_DEPTH_8BIT:
            return ((uint8_t*)vga_framebuffer)[offset];

        case COLOR_DEPTH_16BIT:
            return ((uint16_t*)vga_framebuffer)[offset];

        case COLOR_DEPTH_24BIT:
            {
                uint8_t* pixel = (uint8_t*)vga_framebuffer + (offset * 3);
                return (pixel[0] << 16) | (pixel[1] << 8) | pixel[2];
            }

        case COLOR_DEPTH_32BIT:
            return ((uint32_t*)vga_framebuffer)[offset];

        default:
            return 0;
    }
}

void vga_fill_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint8_t color) {
    for (uint32_t py = y; py < y + height && py < current_vga_height; py++) {
        for (uint32_t px = x; px < x + width && px < current_vga_width; px++) {
            vga_set_pixel(px, py, color);
        }
    }
}

void vga_draw_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint8_t color) {
    // Clamp to screen bounds
    if (x >= current_vga_width || y >= current_vga_height) return;
    if (x + width > current_vga_width) width = current_vga_width - x;
    if (y + height > current_vga_height) height = current_vga_height - y;

    for (uint32_t i = 0; i < width; i++) {
        vga_set_pixel(x + i, y, color);
        vga_set_pixel(x + i, y + height - 1, color);
    }
    for (uint32_t i = 0; i < height; i++) {
        vga_set_pixel(x, y + i, color);
        vga_set_pixel(x + width - 1, y + i, color);
    }
}

void vga_set_desktop_background(void) {
    if (!graphics_initialized) {
        return; // Don't try to set background if graphics not initialized
    }

    // Create a simple gradient background adapted to current resolution
    for (uint32_t y = 0; y < current_vga_height; y++) {
        for (uint32_t x = 0; x < current_vga_width; x++) {
            // A simple gradient from blue to black
            uint8_t gradient_color = (uint8_t)(COLOR_BLUE + (y / (current_vga_height / 16)));
            vga_set_pixel(x, y, gradient_color);
        }
    }
}

void vga_clear(uint8_t color) {
    uint32_t total_pixels = current_vga_width * current_vga_height;
    for (uint32_t i = 0; i < total_pixels; i++) {
        vga_framebuffer[i] = color;
    }
}

// Advanced drawing primitives implementation

// Bresenham's line algorithm
void vga_draw_line(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint8_t color) {
    int32_t dx = (int32_t)x2 - (int32_t)x1;
    int32_t dy = (int32_t)y2 - (int32_t)y1;
    int32_t dx_abs = dx < 0 ? -dx : dx;
    int32_t dy_abs = dy < 0 ? -dy : dy;
    int32_t px = x1;
    int32_t py = y1;
    int32_t sx = dx < 0 ? -1 : 1;
    int32_t sy = dy < 0 ? -1 : 1;

    vga_set_pixel(px, py, color);

    if (dx_abs > dy_abs) {
        int32_t err = dx_abs / 2;
        while (px != x2) {
            err -= dy_abs;
            if (err < 0) {
                py += sy;
                err += dx_abs;
            }
            px += sx;
            vga_set_pixel(px, py, color);
        }
    } else {
        int32_t err = dy_abs / 2;
        while (py != y2) {
            err -= dx_abs;
            if (err < 0) {
                px += sx;
                err += dy_abs;
            }
            py += sy;
            vga_set_pixel(px, py, color);
        }
    }
}

// Midpoint circle algorithm
void vga_draw_circle(uint32_t center_x, uint32_t center_y, uint32_t radius, uint8_t color) {
    int32_t x = radius;
    int32_t y = 0;
    int32_t err = 0;

    while (x >= y) {
        vga_set_pixel(center_x + x, center_y + y, color);
        vga_set_pixel(center_x + y, center_y + x, color);
        vga_set_pixel(center_x - y, center_y + x, color);
        vga_set_pixel(center_x - x, center_y + y, color);
        vga_set_pixel(center_x - x, center_y - y, color);
        vga_set_pixel(center_x - y, center_y - x, color);
        vga_set_pixel(center_x + y, center_y - x, color);
        vga_set_pixel(center_x + x, center_y - y, color);

        if (err <= 0) {
            y += 1;
            err += 2 * y + 1;
        }
        if (err > 0) {
            x -= 1;
            err -= 2 * x + 1;
        }
    }
}

void vga_fill_circle(uint32_t center_x, uint32_t center_y, uint32_t radius, uint8_t color) {
    int32_t x = radius;
    int32_t y = 0;
    int32_t err = 0;

    while (x >= y) {
        // Draw horizontal lines to fill the circle
        vga_draw_horizontal_line(center_x - x, center_y + y, 2 * x + 1, color);
        vga_draw_horizontal_line(center_x - x, center_y - y, 2 * x + 1, color);
        vga_draw_horizontal_line(center_x - y, center_y + x, 2 * y + 1, color);
        vga_draw_horizontal_line(center_x - y, center_y - x, 2 * y + 1, color);

        if (err <= 0) {
            y += 1;
            err += 2 * y + 1;
        }
        if (err > 0) {
            x -= 1;
            err -= 2 * x + 1;
        }
    }
}

// Triangle drawing using line algorithm
void vga_draw_triangle(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t x3, uint32_t y3, uint8_t color) {
    vga_draw_line(x1, y1, x2, y2, color);
    vga_draw_line(x2, y2, x3, y3, color);
    vga_draw_line(x3, y3, x1, y1, color);
}

// Triangle filling using scanline algorithm
void vga_fill_triangle(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t x3, uint32_t y3, uint8_t color) {
    // Sort vertices by y-coordinate
    if (y1 > y2) { uint32_t t = y1; y1 = y2; y2 = t; t = x1; x1 = x2; x2 = t; }
    if (y2 > y3) { uint32_t t = y2; y2 = y3; y3 = t; t = x2; x2 = x3; x3 = t; }
    if (y1 > y2) { uint32_t t = y1; y1 = y2; y2 = t; t = x1; x1 = x2; x2 = t; }

    if (y1 == y3) return; // Degenerate triangle

    int32_t dx1 = x2 - x1;
    int32_t dy1 = y2 - y1;
    int32_t dx2 = x3 - x1;
    int32_t dy2 = y3 - y1;
    int32_t dx3 = x3 - x2;
    int32_t dy3 = y3 - y2;

    int32_t sa = 0, sb = 0;
    if (dy1) sa = dx1 << 16 / dy1;
    if (dy2) sb = dx2 << 16 / dy2;
    if (dy3) {
        int32_t sc = dx3 << 16 / dy3;
        sa = sc;
    }

    int32_t x_left = x1 << 16, x_right = x1 << 16;

    // First half
    for (uint32_t y = y1; y <= y2; y++) {
        vga_draw_horizontal_line(x_left >> 16, y, (x_right >> 16) - (x_left >> 16) + 1, color);
        x_left += sa;
        x_right += sb;
    }

    // Second half
    x_left = x2 << 16;
    for (uint32_t y = y2 + 1; y <= y3; y++) {
        vga_draw_horizontal_line(x_left >> 16, y, (x_right >> 16) - (x_left >> 16) + 1, color);
        x_left += sa;
        x_right += sb;
    }
}

// Utility functions for performance
void vga_draw_horizontal_line(uint32_t x, uint32_t y, uint32_t length, uint8_t color) {
    if (y >= current_vga_height) return;
    uint32_t start_x = x;
    uint32_t end_x = x + length - 1;
    if (start_x >= current_vga_width) return;
    if (end_x >= current_vga_width) end_x = current_vga_width - 1;

    for (uint32_t px = start_x; px <= end_x; px++) {
        vga_set_pixel(px, y, color);
    }
}

void vga_draw_vertical_line(uint32_t x, uint32_t y, uint32_t length, uint8_t color) {
    if (x >= current_vga_width) return;
    uint32_t start_y = y;
    uint32_t end_y = y + length - 1;
    if (start_y >= current_vga_height) return;
    if (end_y >= current_vga_height) end_y = current_vga_height - 1;

    for (uint32_t py = start_y; py <= end_y; py++) {
        vga_set_pixel(x, py, color);
    }
}

// Performance optimized functions
void vga_fast_fill_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint8_t color) {
    // Clamp to screen bounds
    if (x >= current_vga_width || y >= current_vga_height) return;
    if (x + width > current_vga_width) width = current_vga_width - x;
    if (y + height > current_vga_height) height = current_vga_height - y;

    // Use direct memory access for better performance
    uint8_t* start = vga_framebuffer + (y * current_vga_width + x);
    for (uint32_t row = 0; row < height; row++) {
        uint8_t* row_start = start + (row * current_vga_width);
        for (uint32_t col = 0; col < width; col++) {
            row_start[col] = color;
        }
    }
}

void vga_fast_clear(uint8_t color) {
    // Use 32-bit operations for faster clearing when possible
    uint32_t* framebuffer_32 = (uint32_t*)vga_framebuffer;
    uint32_t color_32 = color | (color << 8) | (color << 16) | (color << 24);
    uint32_t pixel_count = current_vga_width * current_vga_height;
    uint32_t dword_count = pixel_count / 4;

    for (uint32_t i = 0; i < dword_count; i++) {
        framebuffer_32[i] = color_32;
    }

    // Handle remaining pixels
    uint32_t remaining = pixel_count % 4;
    uint8_t* remaining_pixels = (uint8_t*)&framebuffer_32[dword_count];
    for (uint32_t i = 0; i < remaining; i++) {
        remaining_pixels[i] = color;
    }
}

// Basic bitmap font rendering (8x8 characters)
static const uint8_t font_8x8[128][8] = {
    // Space (32)
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // Exclamation mark (33)
    {0x18, 0x18, 0x18, 0x18, 0x18, 0x00, 0x18, 0x00},
    // And so on... (simplified for demonstration)
    // This would need the full ASCII character set
};

void vga_draw_char(uint32_t x, uint32_t y, char c, uint8_t color) {
    if (c < 32 || c > 127) return; // Only printable ASCII

    const uint8_t* char_bitmap = font_8x8[(uint8_t)c];

    for (uint32_t row = 0; row < 8; row++) {
        uint8_t row_data = char_bitmap[row];
        for (uint32_t col = 0; col < 8; col++) {
            if (row_data & (1 << (7 - col))) {
                vga_set_pixel(x + col, y + row, color);
            }
        }
    }
}

void vga_draw_string(uint32_t x, uint32_t y, const char* str, uint8_t color) {
    uint32_t current_x = x;
    while (*str) {
        if (*str == '\n') {
            y += 10; // Line height
            current_x = x;
        } else {
            vga_draw_char(current_x, y, *str, color);
            current_x += 8; // Character width
        }
        str++;
    }
}

// Color conversion utilities
uint32_t rgb_to_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    switch (current_color_depth) {
        case COLOR_DEPTH_8BIT:
            // Simple palette approximation - return closest palette index
            // This is a very basic implementation
            return ((r + g + b) / 3) % 256;

        case COLOR_DEPTH_16BIT:
            // 5:6:5 RGB
            return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);

        case COLOR_DEPTH_24BIT:
            // 24-bit RGB
            return (r << 16) | (g << 8) | b;

        case COLOR_DEPTH_32BIT:
            // 32-bit RGBA
            return (a << 24) | (r << 16) | (g << 8) | b;

        default:
            return 0;
    }
}

void color_to_rgb(uint32_t color, uint8_t* r, uint8_t* g, uint8_t* b, uint8_t* a) {
    switch (current_color_depth) {
        case COLOR_DEPTH_8BIT:
            // Simple grayscale approximation
            *r = *g = *b = (uint8_t)color;
            *a = 255;
            break;

        case COLOR_DEPTH_16BIT:
            // 5:6:5 RGB
            *r = ((color >> 11) & 0x1F) << 3;
            *g = ((color >> 5) & 0x3F) << 2;
            *b = (color & 0x1F) << 3;
            *a = 255;
            break;

        case COLOR_DEPTH_24BIT:
            // 24-bit RGB
            *r = (color >> 16) & 0xFF;
            *g = (color >> 8) & 0xFF;
            *b = color & 0xFF;
            *a = 255;
            break;

        case COLOR_DEPTH_32BIT:
            // 32-bit RGBA
            *a = (color >> 24) & 0xFF;
            *r = (color >> 16) & 0xFF;
            *g = (color >> 8) & 0xFF;
            *b = color & 0xFF;
            break;
    }
}

// Software rendering pipeline implementation

render_buffer_t* create_render_buffer(uint32_t width, uint32_t height) {
    render_buffer_t* buffer = (render_buffer_t*)kmalloc(sizeof(render_buffer_t));
    if (!buffer) return 0;

    buffer->width = width;
    buffer->height = height;
    buffer->pixels = (uint32_t*)kmalloc(width * height * sizeof(uint32_t));

    if (!buffer->pixels) {
        kfree(buffer);
        return 0;
    }

    return buffer;
}

void destroy_render_buffer(render_buffer_t* buffer) {
    if (buffer) {
        if (buffer->pixels) {
            kfree(buffer->pixels);
        }
        kfree(buffer);
    }
}

void clear_render_buffer(render_buffer_t* buffer, uint32_t color) {
    if (!buffer || !buffer->pixels) return;

    uint32_t total_pixels = buffer->width * buffer->height;
    for (uint32_t i = 0; i < total_pixels; i++) {
        buffer->pixels[i] = color;
    }
}

void render_buffer_to_screen(render_buffer_t* buffer, uint32_t screen_x, uint32_t screen_y) {
    if (!buffer || !buffer->pixels) return;

    for (uint32_t y = 0; y < buffer->height; y++) {
        for (uint32_t x = 0; x < buffer->width; x++) {
            uint32_t screen_pos_x = screen_x + x;
            uint32_t screen_pos_y = screen_y + y;

            if (screen_pos_x < current_vga_width && screen_pos_y < current_vga_height) {
                uint32_t color = buffer->pixels[y * buffer->width + x];
                vga_set_pixel(screen_pos_x, screen_pos_y, color);
            }
        }
    }
}

// Software rendering primitives

void draw_pixel_software(render_buffer_t* buffer, int32_t x, int32_t y, uint32_t color) {
    if (!buffer || !buffer->pixels) return;
    if (x < 0 || y < 0 || x >= (int32_t)buffer->width || y >= (int32_t)buffer->height) return;

    buffer->pixels[y * buffer->width + x] = color;
}

void draw_line_software(render_buffer_t* buffer, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color) {
    if (!buffer) return;

    int32_t dx = x2 - x1;
    int32_t dy = y2 - y1;
    int32_t dx_abs = dx < 0 ? -dx : dx;
    int32_t dy_abs = dy < 0 ? -dy : dy;
    int32_t px = x1;
    int32_t py = y1;
    int32_t sx = dx < 0 ? -1 : 1;
    int32_t sy = dy < 0 ? -1 : 1;

    draw_pixel_software(buffer, px, py, color);

    if (dx_abs > dy_abs) {
        int32_t err = dx_abs / 2;
        while (px != x2) {
            err -= dy_abs;
            if (err < 0) {
                py += sy;
                err += dx_abs;
            }
            px += sx;
            draw_pixel_software(buffer, px, py, color);
        }
    } else {
        int32_t err = dy_abs / 2;
        while (py != y2) {
            err -= dx_abs;
            if (err < 0) {
                px += sx;
                err += dy_abs;
            }
            py += sy;
            draw_pixel_software(buffer, px, py, color);
        }
    }
}

void draw_rect_software(render_buffer_t* buffer, int32_t x, int32_t y, uint32_t width, uint32_t height, uint32_t color) {
    if (!buffer) return;

    // Draw top and bottom lines
    draw_line_software(buffer, x, y, x + width - 1, y, color);
    draw_line_software(buffer, x, y + height - 1, x + width - 1, y + height - 1, color);

    // Draw left and right lines
    draw_line_software(buffer, x, y, x, y + height - 1, color);
    draw_line_software(buffer, x + width - 1, y, x + width - 1, y + height - 1, color);
}

void fill_rect_software(render_buffer_t* buffer, int32_t x, int32_t y, uint32_t width, uint32_t height, uint32_t color) {
    if (!buffer || !buffer->pixels) return;

    int32_t start_x = x < 0 ? 0 : x;
    int32_t start_y = y < 0 ? 0 : y;
    int32_t end_x = x + width > (int32_t)buffer->width ? buffer->width : x + width;
    int32_t end_y = y + height > (int32_t)buffer->height ? buffer->height : y + height;

    for (int32_t py = start_y; py < end_y; py++) {
        for (int32_t px = start_x; px < end_x; px++) {
            buffer->pixels[py * buffer->width + px] = color;
        }
    }
}

void draw_circle_software(render_buffer_t* buffer, int32_t center_x, int32_t center_y, uint32_t radius, uint32_t color) {
    if (!buffer || radius == 0) return;

    int32_t x = radius;
    int32_t y = 0;
    int32_t err = 0;

    while (x >= y) {
        draw_pixel_software(buffer, center_x + x, center_y + y, color);
        draw_pixel_software(buffer, center_x + y, center_y + x, color);
        draw_pixel_software(buffer, center_x - y, center_y + x, color);
        draw_pixel_software(buffer, center_x - x, center_y + y, color);
        draw_pixel_software(buffer, center_x - x, center_y - y, color);
        draw_pixel_software(buffer, center_x - y, center_y - x, color);
        draw_pixel_software(buffer, center_x + y, center_y - x, color);
        draw_pixel_software(buffer, center_x + x, center_y - y, color);

        if (err <= 0) {
            y += 1;
            err += 2 * y + 1;
        }
        if (err > 0) {
            x -= 1;
            err -= 2 * x + 1;
        }
    }
}

void fill_circle_software(render_buffer_t* buffer, int32_t center_x, int32_t center_y, uint32_t radius, uint32_t color) {
    if (!buffer || radius == 0) return;

    int32_t x = radius;
    int32_t y = 0;
    int32_t err = 0;

    while (x >= y) {
        // Draw horizontal lines to fill the circle
        draw_line_software(buffer, center_x - x, center_y + y, center_x + x, center_y + y, color);
        draw_line_software(buffer, center_x - x, center_y - y, center_x + x, center_y - y, color);
        draw_line_software(buffer, center_x - y, center_y + x, center_x + y, center_y + x, color);
        draw_line_software(buffer, center_x - y, center_y - x, center_x + y, center_y - x, color);

        if (err <= 0) {
            y += 1;
            err += 2 * y + 1;
        }
        if (err > 0) {
            x -= 1;
            err -= 2 * x + 1;
        }
    }
}

// Alpha blending function
uint32_t blend_colors(uint32_t src, uint32_t dst) {
    uint8_t src_a = (src >> 24) & 0xFF;
    uint8_t src_r = (src >> 16) & 0xFF;
    uint8_t src_g = (src >> 8) & 0xFF;
    uint8_t src_b = src & 0xFF;

    uint8_t dst_a = (dst >> 24) & 0xFF;
    uint8_t dst_r = (dst >> 16) & 0xFF;
    uint8_t dst_g = (dst >> 8) & 0xFF;
    uint8_t dst_b = dst & 0xFF;

    // Simple alpha blending
    uint8_t out_a = src_a + dst_a * (255 - src_a) / 255;
    uint8_t out_r = (src_r * src_a + dst_r * dst_a * (255 - src_a) / 255) / out_a;
    uint8_t out_g = (src_g * src_a + dst_g * dst_a * (255 - src_a) / 255) / out_a;
    uint8_t out_b = (src_b * src_a + dst_b * dst_a * (255 - src_a) / 255) / out_a;

    return (out_a << 24) | (out_r << 16) | (out_g << 8) | out_b;
}

// Graphics acceleration optimizations

void vga_blit_buffer(uint32_t* src_buffer, uint32_t src_width, uint32_t src_height,
                     uint32_t dest_x, uint32_t dest_y, uint32_t width, uint32_t height) {
    if (!src_buffer) return;

    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            uint32_t src_x = x;
            uint32_t src_y = y;

            if (src_x < src_width && src_y < src_height) {
                uint32_t color = src_buffer[src_y * src_width + src_x];
                vga_set_pixel(dest_x + x, dest_y + y, color);
            }
        }
    }
}

void vga_blit_buffer_scaled(uint32_t* src_buffer, uint32_t src_width, uint32_t src_height,
                           uint32_t dest_x, uint32_t dest_y, uint32_t dest_width, uint32_t dest_height) {
    if (!src_buffer || dest_width == 0 || dest_height == 0) return;

    for (uint32_t dy = 0; dy < dest_height; dy++) {
        for (uint32_t dx = 0; dx < dest_width; dx++) {
            // Bilinear scaling
            uint32_t sx = (dx * src_width) / dest_width;
            uint32_t sy = (dy * src_height) / dest_height;

            if (sx < src_width && sy < src_height) {
                uint32_t color = src_buffer[sy * src_width + sx];
                vga_set_pixel(dest_x + dx, dest_y + dy, color);
            }
        }
    }
}

// Fast memory operations using inline assembly for better performance
void vga_memcpy_fast(void* dest, const void* src, uint32_t count) {
    __asm__ volatile (
        "rep movsb"
        : "+D"(dest), "+S"(src), "+c"(count)
        :
        : "memory"
    );
}

void vga_memset_fast(void* dest, uint32_t value, uint32_t count) {
    // Simple implementation for freestanding environment
    uint8_t* dest8 = (uint8_t*)dest;
    uint8_t value8 = value & 0xFF;

    for (uint32_t i = 0; i < count; i++) {
        dest8[i] = value8;
    }
}

// Double buffering implementation
double_buffer_t* create_double_buffer(uint32_t width, uint32_t height) {
    double_buffer_t* db = (double_buffer_t*)kmalloc(sizeof(double_buffer_t));
    if (!db) return 0;

    db->width = width;
    db->height = height;
    db->current_buffer = 0;

    uint32_t buffer_size = width * height * sizeof(uint32_t);

    db->front_buffer = (uint32_t*)kmalloc(buffer_size);
    db->back_buffer = (uint32_t*)kmalloc(buffer_size);

    if (!db->front_buffer || !db->back_buffer) {
        if (db->front_buffer) kfree(db->front_buffer);
        if (db->back_buffer) kfree(db->back_buffer);
        kfree(db);
        return 0;
    }

    // Clear both buffers
    vga_memset_fast(db->front_buffer, 0, buffer_size);
    vga_memset_fast(db->back_buffer, 0, buffer_size);

    return db;
}

void destroy_double_buffer(double_buffer_t* db) {
    if (db) {
        if (db->front_buffer) kfree(db->front_buffer);
        if (db->back_buffer) kfree(db->back_buffer);
        kfree(db);
    }
}

void swap_buffers(double_buffer_t* db) {
    if (!db) return;
    db->current_buffer = 1 - db->current_buffer;
}

uint32_t* get_current_buffer(double_buffer_t* db) {
    if (!db) return 0;
    return db->current_buffer ? db->back_buffer : db->front_buffer;
}

void present_buffer(double_buffer_t* db) {
    if (!db) return;

    uint32_t* current = get_current_buffer(db);
    if (current) {
        vga_blit_buffer(current, db->width, db->height, 0, 0, db->width, db->height);
    }
}

