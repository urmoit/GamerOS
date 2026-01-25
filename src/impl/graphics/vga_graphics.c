// src/impl/graphics/vga_graphics.c - DIAGNOSTIC VERSION

#include "../../intf/graphics.h"
#include "../../intf/ui.h"
#include "../../intf/font.h"
#include "../../intf/ports.h"

// Global video mode state
uint32_t current_vga_width = VGA_WIDTH;
uint32_t current_vga_height = VGA_HEIGHT;
uint32_t current_color_depth = COLOR_DEPTH_8BIT;
vga_mode_t current_vga_mode = VGA_MODE_13H;
int graphics_initialized = 0;

// VGA framebuffer - use volatile to prevent optimization
static volatile uint8_t* vga_framebuffer = (volatile uint8_t*)0xA0000;

// Force a write to framebuffer that won't be optimized away
static inline void force_write_pixel(uint32_t offset, uint8_t color) {
    volatile uint8_t* fb = (volatile uint8_t*)0xA0000;
    fb[offset] = color;
    // Read it back to ensure write completed
    volatile uint8_t verify = fb[offset];
    (void)verify;
}

// Initialize VGA palette - SIMPLIFIED AND ROBUST
static void vga_init_palette(void) {
    // First, set black for color 0
    outb(0x3C8, 0x00);
    outb(0x3C9, 0x00);
    outb(0x3C9, 0x00);
    outb(0x3C9, 0x00);

    // Blue - color 1
    outb(0x3C8, 0x01);
    outb(0x3C9, 0x00);
    outb(0x3C9, 0x00);
    outb(0x3C9, 0x3F);

    // Green - color 2
    outb(0x3C8, 0x02);
    outb(0x3C9, 0x00);
    outb(0x3C9, 0x3F);
    outb(0x3C9, 0x00);

    // Cyan - color 3
    outb(0x3C8, 0x03);
    outb(0x3C9, 0x00);
    outb(0x3C9, 0x3F);
    outb(0x3C9, 0x3F);

    // Red - color 4
    outb(0x3C8, 0x04);
    outb(0x3C9, 0x3F);
    outb(0x3C9, 0x00);
    outb(0x3C9, 0x00);

    // Magenta - color 5
    outb(0x3C8, 0x05);
    outb(0x3C9, 0x3F);
    outb(0x3C9, 0x00);
    outb(0x3C9, 0x3F);

    // Brown - color 6
    outb(0x3C8, 0x06);
    outb(0x3C9, 0x2A);
    outb(0x3C9, 0x15);
    outb(0x3C9, 0x00);

    // Light gray - color 7
    outb(0x3C8, 0x07);
    outb(0x3C9, 0x2A);
    outb(0x3C9, 0x2A);
    outb(0x3C9, 0x2A);

    // Dark gray - color 8
    outb(0x3C8, 0x08);
    outb(0x3C9, 0x15);
    outb(0x3C9, 0x15);
    outb(0x3C9, 0x15);

    // Bright blue - color 9
    outb(0x3C8, 0x09);
    outb(0x3C9, 0x15);
    outb(0x3C9, 0x15);
    outb(0x3C9, 0x3F);

    // Bright green - color 10
    outb(0x3C8, 0x0A);
    outb(0x3C9, 0x15);
    outb(0x3C9, 0x3F);
    outb(0x3C9, 0x15);

    // Bright cyan - color 11
    outb(0x3C8, 0x0B);
    outb(0x3C9, 0x15);
    outb(0x3C9, 0x3F);
    outb(0x3C9, 0x3F);

    // Bright red - color 12
    outb(0x3C8, 0x0C);
    outb(0x3C9, 0x3F);
    outb(0x3C9, 0x15);
    outb(0x3C9, 0x15);

    // Bright magenta - color 13
    outb(0x3C8, 0x0D);
    outb(0x3C9, 0x3F);
    outb(0x3C9, 0x15);
    outb(0x3C9, 0x3F);

    // Yellow - color 14
    outb(0x3C8, 0x0E);
    outb(0x3C9, 0x3F);
    outb(0x3C9, 0x3F);
    outb(0x3C9, 0x00);

    // White - color 15
    outb(0x3C8, 0x0F);
    outb(0x3C9, 0x3F);
    outb(0x3C9, 0x3F);
    outb(0x3C9, 0x3F);

    // Fill rest with grayscale
    for (int i = 16; i < 256; i++) {
        uint8_t intensity = (i * 63) / 255;
        outb(0x3C8, i);
        outb(0x3C9, intensity);
        outb(0x3C9, intensity);
        outb(0x3C9, intensity);
    }
}

void vga_init_mode13(void) {
    // Mode 13h is already set in boot.asm
    vga_framebuffer = (volatile uint8_t*)0xA0000;
    current_vga_width = 320;
    current_vga_height = 200;
    current_vga_mode = VGA_MODE_13H;
    current_color_depth = COLOR_DEPTH_8BIT;

    // Initialize palette
    vga_init_palette();

    // Mark as initialized
    graphics_initialized = 1;
}

void vga_init_mode12h(void) {
    // Mode 12h: 640x480x16 (4-bit color, planar mode)
    // Each byte contains 8 pixels (1 plane per color)
    // 4 planes total: plane 0=bit 0, plane 1=bit 1, plane 2=bit 2, plane 3=bit 3
    
    vga_framebuffer = (volatile uint8_t*)0xA0000;
    current_vga_width = 640;
    current_vga_height = 480;
    current_vga_mode = VGA_MODE_12H;
    current_color_depth = COLOR_DEPTH_8BIT;  // 16 colors (4 bits per pixel)
    
    // Configure graphics controller for planar mode
    // Graphics Mode Register (0x3CE/0x3CF)
    outb(0x3CE, 0x05);  // Graphics Mode register
    outb(0x3CF, 0x00);  // Mode 0: write to all planes
    
    // Sequencer registers for planar access
    outb(0x3C4, 0x02);  // Map Mask register
    outb(0x3C5, 0x0F);  // Enable all 4 planes for writing
    
    graphics_initialized = 1;
}

int vga_set_mode(vga_mode_t mode) {
    switch (mode) {
        case VGA_MODE_13H:
            vga_init_mode13();
            return 1;
        case VGA_MODE_12H:
            vga_init_mode12h();
            return 1;
        case VGA_MODE_101H:
            // 640x480x256 - would require VESA calls
            return 0;
        case VGA_MODE_103H:
            // 800x600x256 - would require VESA calls
            return 0;
        case VGA_MODE_118H:
            // 1024x768x24 - would require VESA calls
            return 0;
        default:
            return 0;
    }
}

void vga_set_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (current_vga_mode == VGA_MODE_13H) {
        if (x >= 320 || y >= 200) return;
        uint32_t offset = y * 320 + x;
        force_write_pixel(offset, (uint8_t)color);
    } else if (current_vga_mode == VGA_MODE_12H) {
        if (x >= 640 || y >= 480) return;
        
        // Mode 12h: planar mode - 8 pixels per byte, 16 colors (4 bits per pixel)
        uint32_t byte_offset = y * 80 + (x / 8);
        uint8_t bit_position = 7 - (x % 8);
        uint8_t pixel_mask = 1 << bit_position;
        uint8_t color_byte = (uint8_t)color & 0x0F;
        
        volatile uint8_t* fb = (volatile uint8_t*)0xA0000;
        
        // Write each plane separately using only sequencer mask
        for (int plane = 0; plane < 4; plane++) {
            // Set sequencer to write only to this plane
            outb(0x3C4, 0x02);  // Map Mask register
            outb(0x3C5, 1 << plane);
            
            // Check if this plane should have the bit set
            if (color_byte & (1 << plane)) {
                // Set the bit: read-modify-write
                uint8_t current = fb[byte_offset];
                current |= pixel_mask;
                fb[byte_offset] = current;
            } else {
                // Clear the bit: read-modify-write
                uint8_t current = fb[byte_offset];
                current &= ~pixel_mask;
                fb[byte_offset] = current;
            }
        }
        
        // Reset sequencer to all planes
        outb(0x3C4, 0x02);
        outb(0x3C5, 0x0F);
        
        __asm__ volatile("mfence" ::: "memory");
    }
}

uint32_t vga_get_pixel(uint32_t x, uint32_t y) {
    if (current_vga_mode == VGA_MODE_13H) {
        if (x >= 320 || y >= 200) return 0;
        volatile uint8_t* fb = (volatile uint8_t*)0xA0000;
        return fb[y * 320 + x];
    } else if (current_vga_mode == VGA_MODE_12H) {
        if (x >= 640 || y >= 480) return 0;
        
        uint32_t byte_offset = y * 80 + (x / 8);
        uint8_t bit_position = 7 - (x % 8);
        uint8_t pixel_mask = 1 << bit_position;
        
        volatile uint8_t* fb = (volatile uint8_t*)0xA0000;
        uint32_t color = 0;
        
        // Read each plane
        for (int plane = 0; plane < 4; plane++) {
            outb(0x3CE, 0x04);  // Graphics Read Map Select
            outb(0x3CF, plane);
            
            if (fb[byte_offset] & pixel_mask) {
                color |= (1 << plane);
            }
        }
        
        return color;
    }
    return 0;
}

void vga_fill_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint8_t color) {
    if (current_vga_mode == VGA_MODE_13H) {
        // Clamp to screen
        if (x >= 320 || y >= 200) return;
        if (x + width > 320) width = 320 - x;
        if (y + height > 200) height = 200 - y;

        volatile uint8_t* fb = (volatile uint8_t*)0xA0000;

        for (uint32_t py = y; py < y + height; py++) {
            for (uint32_t px = x; px < x + width; px++) {
                fb[py * 320 + px] = color;
            }
        }

        // Memory barrier
        __asm__ volatile("mfence" ::: "memory");
    } else if (current_vga_mode == VGA_MODE_12H) {
        // Clamp to screen
        if (x >= 640 || y >= 480) return;
        if (x + width > 640) width = 640 - x;
        if (y + height > 480) height = 480 - y;

        for (uint32_t py = y; py < y + height; py++) {
            for (uint32_t px = x; px < x + width; px++) {
                vga_set_pixel(px, py, color);
            }
        }
    }
}

void vga_clear(uint8_t color) {
    if (current_vga_mode == VGA_MODE_13H) {
        volatile uint8_t* fb = (volatile uint8_t*)0xA0000;

        for (uint32_t i = 0; i < 320 * 200; i++) {
            fb[i] = color;
        }

        __asm__ volatile("mfence" ::: "memory");
    } else if (current_vga_mode == VGA_MODE_12H) {
        // Mode 12h: optimized clear - write entire screen at once
        volatile uint8_t* fb = (volatile uint8_t*)0xA0000;
        uint8_t color_byte = color & 0x0F;
        uint32_t screen_bytes = 640 * 480 / 8;  // 38400 bytes
        
        // For each plane, write all pixels if that plane bit is set in color
        for (int plane = 0; plane < 4; plane++) {
            if (color_byte & (1 << plane)) {
                // Set sequencer to write only to this plane
                outb(0x3C4, 0x02);
                outb(0x3C5, 1 << plane);
                
                // Fill entire screen with 0xFF (all pixels set in this plane)
                for (uint32_t i = 0; i < screen_bytes; i++) {
                    fb[i] = 0xFF;
                }
            } else {
                // Set sequencer to write only to this plane
                outb(0x3C4, 0x02);
                outb(0x3C5, 1 << plane);
                
                // Clear entire screen (all pixels clear in this plane)
                for (uint32_t i = 0; i < screen_bytes; i++) {
                    fb[i] = 0x00;
                }
            }
        }
        
        // Reset to all planes
        outb(0x3C4, 0x02);
        outb(0x3C5, 0x0F);
        
        __asm__ volatile("mfence" ::: "memory");
    }
}

void vga_draw_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint8_t color) {
    // Top and bottom
    for (uint32_t i = 0; i < width && x + i < current_vga_width; i++) {
        vga_set_pixel(x + i, y, color);
        if (y + height - 1 < current_vga_height) {
            vga_set_pixel(x + i, y + height - 1, color);
        }
    }
    // Left and right
    for (uint32_t i = 0; i < height && y + i < current_vga_height; i++) {
        vga_set_pixel(x, y + i, color);
        if (x + width - 1 < current_vga_width) {
            vga_set_pixel(x + width - 1, y + i, color);
        }
    }
}

void vga_draw_char(uint32_t x, uint32_t y, char c, uint8_t color) {
    if ((uint8_t)c < 32 || (uint8_t)c > 126) return;

    const uint8_t* char_bitmap = font_8x8[(uint8_t)c - 32];

    for (uint32_t row = 0; row < 8; row++) {
        uint8_t row_data = char_bitmap[row];
        for (uint32_t col = 0; col < 8; col++) {
            if (row_data & (1 << (7 - col))) {
                if (x + col < current_vga_width && y + row < current_vga_height) {
                    vga_set_pixel(x + col, y + row, color);
                }
            }
        }
    }
}

void vga_draw_string(uint32_t x, uint32_t y, const char* str, uint8_t color) {
    if (!str) return;

    uint32_t current_x = x;
    while (*str) {
        if (*str == '\n') {
            y += 10;
            current_x = x;
        } else {
            if (current_x < 320 && y < 200) {
                vga_draw_char(current_x, y, *str, color);
            }
            current_x += 8;
        }
        str++;
    }
}

// Minimal stubs for other functions
void vga_draw_line(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint8_t color) {
    // Bresenham's line algorithm
    int32_t dx = (x2 > x1) ? (x2 - x1) : (x1 - x2);
    int32_t dy = (y2 > y1) ? (y2 - y1) : (y1 - y2);
    int32_t sx = (x1 < x2) ? 1 : -1;
    int32_t sy = (y1 < y2) ? 1 : -1;
    int32_t err = dx - dy;

    int32_t x = x1;
    int32_t y = y1;

    while (1) {
        if (x >= 0 && x < (int32_t)current_vga_width && y >= 0 && y < (int32_t)current_vga_height) {
            vga_set_pixel(x, y, color);
        }

        if (x == (int32_t)x2 && y == (int32_t)y2) {
            break;
        }

        int32_t e2 = err * 2;
        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }
        if (e2 < dx) {
            err += dx;
            y += sy;
        }
    }
}

void vga_draw_circle(uint32_t cx, uint32_t cy, uint32_t r, uint8_t color) {
    // Midpoint circle algorithm
    int32_t x = r;
    int32_t y = 0;
    int32_t d = 3 - 2 * r;

    while (x >= y) {
        // Draw 8 symmetric points
        if ((int32_t)cx + x < (int32_t)current_vga_width && (int32_t)cy + y < (int32_t)current_vga_height) vga_set_pixel(cx + x, cy + y, color);
        if ((int32_t)cx - x >= 0 && (int32_t)cy + y < (int32_t)current_vga_height) vga_set_pixel(cx - x, cy + y, color);
        if ((int32_t)cx + x < (int32_t)current_vga_width && (int32_t)cy - y >= 0) vga_set_pixel(cx + x, cy - y, color);
        if ((int32_t)cx - x >= 0 && (int32_t)cy - y >= 0) vga_set_pixel(cx - x, cy - y, color);
        if ((int32_t)cx + y < (int32_t)current_vga_width && (int32_t)cy + x < (int32_t)current_vga_height) vga_set_pixel(cx + y, cy + x, color);
        if ((int32_t)cx - y >= 0 && (int32_t)cy + x < (int32_t)current_vga_height) vga_set_pixel(cx - y, cy + x, color);
        if ((int32_t)cx + y < (int32_t)current_vga_width && (int32_t)cy - x >= 0) vga_set_pixel(cx + y, cy - x, color);
        if ((int32_t)cx - y >= 0 && (int32_t)cy - x >= 0) vga_set_pixel(cx - y, cy - x, color);

        if (d < 0) {
            d = d + 4 * y + 6;
        } else {
            d = d + 4 * (y - x) + 10;
            x--;
        }
        y++;
    }
}

void vga_fill_circle(uint32_t cx, uint32_t cy, uint32_t r, uint8_t color) {
    // Midpoint circle algorithm with horizontal line filling
    int32_t x = r;
    int32_t y = 0;
    int32_t d = 3 - 2 * r;

    while (x >= y) {
        // Draw horizontal lines for each symmetric pair
        vga_draw_horizontal_line(cx - x, cy + y, 2 * x + 1, color);
        vga_draw_horizontal_line(cx - x, cy - y, 2 * x + 1, color);
        vga_draw_horizontal_line(cx - y, cy + x, 2 * y + 1, color);
        vga_draw_horizontal_line(cx - y, cy - x, 2 * y + 1, color);

        if (d < 0) {
            d = d + 4 * y + 6;
        } else {
            d = d + 4 * (y - x) + 10;
            x--;
        }
        y++;
    }
}

void vga_draw_triangle(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t x3, uint32_t y3, uint8_t color) {
    (void)x1; (void)y1; (void)x2; (void)y2; (void)x3; (void)y3; (void)color;
}

void vga_fill_triangle(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t x3, uint32_t y3, uint8_t color) {
    (void)x1; (void)y1; (void)x2; (void)y2; (void)x3; (void)y3; (void)color;
}

void vga_draw_horizontal_line(uint32_t x, uint32_t y, uint32_t len, uint8_t color) {
    for (uint32_t i = 0; i < len && x + i < current_vga_width; i++) {
        vga_set_pixel(x + i, y, color);
    }
}

void vga_draw_vertical_line(uint32_t x, uint32_t y, uint32_t len, uint8_t color) {
    for (uint32_t i = 0; i < len && y + i < current_vga_height; i++) {
        vga_set_pixel(x, y + i, color);
    }
}

void vga_fast_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t c) {
    vga_fill_rect(x, y, w, h, c);
}

void vga_fast_clear(uint8_t c) {
    vga_clear(c);
}

void vga_set_desktop_background(void) {
    vga_clear(0x01); // Blue
}

uint32_t rgb_to_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    (void)a;
    return ((r >> 5) << 5) | ((g >> 5) << 2) | (b >> 6);
}

void color_to_rgb(uint32_t color, uint8_t* r, uint8_t* g, uint8_t* b, uint8_t* a) {
    *r = (color & 0xE0);
    *g = (color & 0x1C) << 3;
    *b = (color & 0x03) << 6;
    *a = 255;
}

// Stubs for remaining functions
render_buffer_t* create_render_buffer(uint32_t w, uint32_t h) { (void)w; (void)h; return 0; }
void destroy_render_buffer(render_buffer_t* b) { (void)b; }
void clear_render_buffer(render_buffer_t* b, uint32_t c) { (void)b; (void)c; }
void render_buffer_to_screen(render_buffer_t* b, uint32_t x, uint32_t y) { (void)b; (void)x; (void)y; }
void draw_pixel_software(render_buffer_t* b, int32_t x, int32_t y, uint32_t c) { (void)b; (void)x; (void)y; (void)c; }
void draw_line_software(render_buffer_t* b, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t c) { (void)b; (void)x1; (void)y1; (void)x2; (void)y2; (void)c; }
void draw_rect_software(render_buffer_t* b, int32_t x, int32_t y, uint32_t w, uint32_t h, uint32_t c) { (void)b; (void)x; (void)y; (void)w; (void)h; (void)c; }
void fill_rect_software(render_buffer_t* b, int32_t x, int32_t y, uint32_t w, uint32_t h, uint32_t c) { (void)b; (void)x; (void)y; (void)w; (void)h; (void)c; }
void draw_circle_software(render_buffer_t* b, int32_t cx, int32_t cy, uint32_t r, uint32_t c) { (void)b; (void)cx; (void)cy; (void)r; (void)c; }
void fill_circle_software(render_buffer_t* b, int32_t cx, int32_t cy, uint32_t r, uint32_t c) { (void)b; (void)cx; (void)cy; (void)r; (void)c; }
uint32_t blend_colors(uint32_t src, uint32_t dst) { (void)src; (void)dst; return 0; }
void vga_blit_buffer(uint32_t* s, uint32_t sw, uint32_t sh, uint32_t dx, uint32_t dy, uint32_t w, uint32_t h) { (void)s; (void)sw; (void)sh; (void)dx; (void)dy; (void)w; (void)h; }
void vga_blit_buffer_scaled(uint32_t* s, uint32_t sw, uint32_t sh, uint32_t dx, uint32_t dy, uint32_t dw, uint32_t dh) { (void)s; (void)sw; (void)sh; (void)dx; (void)dy; (void)dw; (void)dh; }
void vga_memcpy_fast(void* d, const void* s, uint32_t c) { (void)d; (void)s; (void)c; }
void vga_memset_fast(void* d, uint32_t v, uint32_t c) { (void)d; (void)v; (void)c; }
double_buffer_t* create_double_buffer(uint32_t w, uint32_t h) { (void)w; (void)h; return 0; }
void destroy_double_buffer(double_buffer_t* db) { (void)db; }
void swap_buffers(double_buffer_t* db) { (void)db; }
uint32_t* get_current_buffer(double_buffer_t* db) { (void)db; return 0; }
void present_buffer(double_buffer_t* db) { (void)db; }
void vga_draw_gradient_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t cs, uint32_t ce, uint8_t v) { (void)x; (void)y; (void)w; (void)h; (void)cs; (void)ce; (void)v; }
void vga_draw_radial_gradient(uint32_t cx, uint32_t cy, uint32_t r, uint32_t cc, uint32_t ce) { (void)cx; (void)cy; (void)r; (void)cc; (void)ce; }
void vga_draw_shadow(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t sc, uint8_t blur) { (void)x; (void)y; (void)w; (void)h; (void)sc; (void)blur; }
void vga_draw_glow(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t gc, uint8_t i) { (void)x; (void)y; (void)w; (void)h; (void)gc; (void)i; }
void vga_draw_rounded_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t r, uint8_t c) { (void)x; (void)y; (void)w; (void)h; (void)r; (void)c; }
void vga_fill_rounded_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t r, uint8_t c) { (void)x; (void)y; (void)w; (void)h; (void)r; (void)c; }
void vga_draw_bezier_curve(uint32_t x1, uint32_t y1, uint32_t cx1, uint32_t cy1, uint32_t cx2, uint32_t cy2, uint32_t x2, uint32_t y2, uint8_t c, uint8_t t) { (void)x1; (void)y1; (void)cx1; (void)cy1; (void)cx2; (void)cy2; (void)x2; (void)y2; (void)c; (void)t; }
void vga_draw_aa_line(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t c) { (void)x1; (void)y1; (void)x2; (void)y2; (void)c; }
void vga_draw_aa_circle(uint32_t cx, uint32_t cy, uint32_t r, uint32_t c) { (void)cx; (void)cy; (void)r; (void)c; }
void vga_draw_text_shadow(uint32_t x, uint32_t y, const char* s, uint8_t tc, uint8_t sc) { (void)x; (void)y; (void)s; (void)tc; (void)sc; }
void vga_draw_text_outline(uint32_t x, uint32_t y, const char* s, uint8_t tc, uint8_t oc) { (void)x; (void)y; (void)s; (void)tc; (void)oc; }
image_t* create_image(uint32_t w, uint32_t h) { (void)w; (void)h; return 0; }
void destroy_image(image_t* img) { (void)img; }
void draw_image(uint32_t x, uint32_t y, image_t* img) { (void)x; (void)y; (void)img; }
void draw_image_scaled(uint32_t x, uint32_t y, uint32_t dw, uint32_t dh, image_t* img) { (void)x; (void)y; (void)dw; (void)dh; (void)img; }
uint32_t blend_pixel(uint32_t src, uint32_t dst) { (void)src; (void)dst; return 0; }

// Mouse cursor drawing functions
// Draw a simple arrow cursor
void vga_draw_mouse_arrow(int32_t x, int32_t y, uint8_t color) {
    // Clamp to screen bounds
    if (x < 0 || x >= (int32_t)current_vga_width || y < 0 || y >= (int32_t)current_vga_height) {
        return;
    }
    
    // Draw arrow cursor (small triangle)
    // Main vertical line
    for (int i = 0; i < 8; i++) {
        if (y + i < (int32_t)current_vga_height) {
            vga_set_pixel(x, y + i, color);
        }
    }
    
    // Diagonal lines for arrow shape
    for (int i = 0; i < 6; i++) {
        if (y + i < (int32_t)current_vga_height && x + i < (int32_t)current_vga_width) {
            vga_set_pixel(x + i, y + 2 + i, color);
        }
        if (y + i < (int32_t)current_vga_height && x - i >= 0) {
            vga_set_pixel(x - i + 1, y + 2 + i, color);
        }
    }
}

// Draw a simple crosshair/plus cursor
void vga_draw_mouse_cursor(int32_t x, int32_t y, uint8_t color) {
    // Draw horizontal line
    for (int i = -4; i <= 4; i++) {
        if (x + i >= 0 && x + i < (int32_t)current_vga_width && 
            y >= 0 && y < (int32_t)current_vga_height) {
            vga_set_pixel(x + i, y, color);
        }
    }
    
    // Draw vertical line
    for (int i = -4; i <= 4; i++) {
        if (x >= 0 && x < (int32_t)current_vga_width && 
            y + i >= 0 && y + i < (int32_t)current_vga_height) {
            vga_set_pixel(x, y + i, color);
        }
    }
}

// Erase mouse cursor area by redrawing background
void vga_erase_mouse_area(int32_t x, int32_t y) {
    // Erase a 10x10 area around the mouse cursor
    for (int dy = -5; dy <= 5; dy++) {
        for (int dx = -5; dx <= 5; dx++) {
            if (x + dx >= 0 && x + dx < (int32_t)current_vga_width && 
                y + dy >= 0 && y + dy < (int32_t)current_vga_height) {
                // Redraw background (use UI background color)
                vga_set_pixel(x + dx, y + dy, 0x01);  // Blue background
            }
        }
    }
}

int vga_init_mode101h(void) { return 0; }  // TODO: Implement with VESA calls
int vga_init_mode103h(void) { return 0; }  // TODO: Implement with VESA calls
int vga_init_mode118h(void) { return 0; }  // TODO: Implement with VESA calls
// TODO: Implement double buffering for smoother graphics
// TODO: Add hardware-accelerated graphics operations
// TODO: Implement image loading and rendering functions