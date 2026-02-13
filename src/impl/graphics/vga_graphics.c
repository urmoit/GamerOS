#include "../../intf/graphics.h"
#include "../../intf/ports.h"
#include "../../intf/font.h"

// Current mode settings
uint32_t current_width = 320;
uint32_t current_height = 200;
uint8_t* framebuffer = (uint8_t*)0xA0000;
int graphics_mode = 0;

// Back buffer (max 800x600)
static uint8_t back_buffer[800 * 600];

// VGA Palette setup
void vga_set_palette(void) {
    // Set standard 16 colors
    for (int i = 0; i < 16; i++) {
        outb(0x3C8, i);
        switch(i) {
            case 0: outb(0x3C9, 0); outb(0x3C9, 0); outb(0x3C9, 0); break; // Black
            case 1: outb(0x3C9, 0); outb(0x3C9, 0); outb(0x3C9, 42); break; // Blue
            case 2: outb(0x3C9, 0); outb(0x3C9, 42); outb(0x3C9, 0); break; // Green
            case 3: outb(0x3C9, 0); outb(0x3C9, 42); outb(0x3C9, 42); break; // Cyan
            case 4: outb(0x3C9, 42); outb(0x3C9, 0); outb(0x3C9, 0); break; // Red
            case 5: outb(0x3C9, 42); outb(0x3C9, 0); outb(0x3C9, 42); break; // Magenta
            case 6: outb(0x3C9, 42); outb(0x3C9, 21); outb(0x3C9, 0); break; // Brown
            case 7: outb(0x3C9, 42); outb(0x3C9, 42); outb(0x3C9, 42); break; // Light Gray
            case 8: outb(0x3C9, 21); outb(0x3C9, 21); outb(0x3C9, 21); break; // Dark Gray
            case 9: outb(0x3C9, 21); outb(0x3C9, 21); outb(0x3C9, 63); break; // Light Blue
            case 10: outb(0x3C9, 21); outb(0x3C9, 63); outb(0x3C9, 21); break; // Light Green
            case 11: outb(0x3C9, 21); outb(0x3C9, 63); outb(0x3C9, 63); break; // Light Cyan
            case 12: outb(0x3C9, 63); outb(0x3C9, 21); outb(0x3C9, 21); break; // Light Red
            case 13: outb(0x3C9, 63); outb(0x3C9, 21); outb(0x3C9, 63); break; // Light Magenta
            case 14: outb(0x3C9, 63); outb(0x3C9, 63); outb(0x3C9, 21); break; // Yellow
            case 15: outb(0x3C9, 63); outb(0x3C9, 63); outb(0x3C9, 63); break; // White
        }
    }
    
    // XP Desktop blue (color 0x39)
    outb(0x3C8, 0x39);
    outb(0x3C9, 36);  // R
    outb(0x3C9, 82);  // G
    outb(0x3C9, 120); // B
}

// Set standard VGA 320x200 mode
void vga_set_mode_13h(void) {
    // BIOS interrupts are not valid in long mode. The boot stage sets mode 13h.
    current_width = 320;
    current_height = 200;
    framebuffer = (uint8_t*)0xA0000;
    graphics_mode = 0;
    
    vga_set_palette();
}

// Set VESA mode
int vesa_set_mode(uint16_t mode) {
    (void)mode;
    // Long-mode kernel cannot safely invoke VESA BIOS services.
    return 0;
}

// Set video mode
void set_video_mode(video_mode_t mode) {
    switch (mode) {
        case MODE_VGA_320x200:
            vga_set_mode_13h();
            break;
        case MODE_VESA_640x480:
            if (!vesa_set_mode(0x101)) {
                vga_set_mode_13h(); // Fallback
            }
            break;
        case MODE_VESA_800x600:
            if (!vesa_set_mode(0x103)) {
                vga_set_mode_13h(); // Fallback
            }
            break;
    }
}

// Clear back buffer
void clear_screen(uint8_t color) {
    for (uint32_t i = 0; i < current_width * current_height; i++) {
        back_buffer[i] = color;
    }
}

// Draw pixel to back buffer
void draw_pixel(int x, int y, uint8_t color) {
    if (x < 0 || x >= (int)current_width || y < 0 || y >= (int)current_height) return;
    back_buffer[y * current_width + x] = color;
}

// Fill rectangle
void fill_rect(int x, int y, int w, int h, uint8_t color) {
    for (int yy = y; yy < y + h && yy < (int)current_height; yy++) {
        for (int xx = x; xx < x + w && xx < (int)current_width; xx++) {
            if (xx >= 0 && yy >= 0) {
                back_buffer[yy * current_width + xx] = color;
            }
        }
    }
}

// Draw rectangle outline
void draw_rect(int x, int y, int w, int h, uint8_t color) {
    for (int xx = x; xx < x + w && xx < (int)current_width; xx++) {
        if (xx >= 0 && y >= 0 && y < (int)current_height) {
            back_buffer[y * current_width + xx] = color;
        }
        if (xx >= 0 && y + h - 1 >= 0 && y + h - 1 < (int)current_height) {
            back_buffer[(y + h - 1) * current_width + xx] = color;
        }
    }
    for (int yy = y; yy < y + h && yy < (int)current_height; yy++) {
        if (x >= 0 && yy >= 0) {
            back_buffer[yy * current_width + x] = color;
        }
        if (x + w - 1 >= 0 && x + w - 1 < (int)current_width && yy >= 0) {
            back_buffer[yy * current_width + (x + w - 1)] = color;
        }
    }
}

// Draw line
void draw_line(int x1, int y1, int x2, int y2, uint8_t color) {
    int dx = (x2 > x1) ? (x2 - x1) : (x1 - x2);
    int dy = (y2 > y1) ? (y2 - y1) : (y1 - y2);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;
    
    while (1) {
        draw_pixel(x1, y1, color);
        if (x1 == x2 && y1 == y2) break;
        int e2 = err * 2;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 < dx) { err += dx; y1 += sy; }
    }
}

// Simple 8x8 font patterns
static const uint8_t simple_font[128][8] = {
    ['A'] = {0x7E,0x09,0x09,0x09,0x7E,0x00,0x00,0x00},
    ['B'] = {0x7F,0x49,0x49,0x49,0x36,0x00,0x00,0x00},
    ['C'] = {0x3E,0x41,0x41,0x41,0x22,0x00,0x00,0x00},
    ['D'] = {0x7F,0x41,0x41,0x22,0x1C,0x00,0x00,0x00},
    ['E'] = {0x7F,0x49,0x49,0x49,0x41,0x00,0x00,0x00},
    ['F'] = {0x7F,0x09,0x09,0x09,0x01,0x00,0x00,0x00},
    ['G'] = {0x3E,0x41,0x49,0x49,0x7A,0x00,0x00,0x00},
    ['H'] = {0x7F,0x08,0x08,0x08,0x7F,0x00,0x00,0x00},
    ['I'] = {0x00,0x41,0x7F,0x41,0x00,0x00,0x00,0x00},
    ['J'] = {0x20,0x40,0x41,0x3F,0x01,0x00,0x00,0x00},
    ['K'] = {0x7F,0x08,0x14,0x22,0x41,0x00,0x00,0x00},
    ['L'] = {0x7F,0x40,0x40,0x40,0x40,0x00,0x00,0x00},
    ['M'] = {0x7F,0x02,0x0C,0x02,0x7F,0x00,0x00,0x00},
    ['N'] = {0x7F,0x04,0x08,0x10,0x7F,0x00,0x00,0x00},
    ['O'] = {0x3E,0x41,0x41,0x41,0x3E,0x00,0x00,0x00},
    ['P'] = {0x7F,0x09,0x09,0x09,0x06,0x00,0x00,0x00},
    ['Q'] = {0x3E,0x41,0x51,0x21,0x5E,0x00,0x00,0x00},
    ['R'] = {0x7F,0x09,0x19,0x29,0x46,0x00,0x00,0x00},
    ['S'] = {0x46,0x49,0x49,0x49,0x31,0x00,0x00,0x00},
    ['T'] = {0x01,0x01,0x7F,0x01,0x01,0x00,0x00,0x00},
    ['U'] = {0x3F,0x40,0x40,0x40,0x3F,0x00,0x00,0x00},
    ['V'] = {0x1F,0x20,0x40,0x20,0x1F,0x00,0x00,0x00},
    ['W'] = {0x3F,0x40,0x38,0x40,0x3F,0x00,0x00,0x00},
    ['X'] = {0x63,0x14,0x08,0x14,0x63,0x00,0x00,0x00},
    ['Y'] = {0x07,0x08,0x70,0x08,0x07,0x00,0x00,0x00},
    ['Z'] = {0x61,0x51,0x49,0x45,0x43,0x00,0x00,0x00},
    ['a'] = {0x20,0x54,0x54,0x54,0x78,0x00,0x00,0x00},
    ['b'] = {0x7F,0x48,0x44,0x44,0x38,0x00,0x00,0x00},
    ['c'] = {0x38,0x44,0x44,0x44,0x20,0x00,0x00,0x00},
    ['d'] = {0x38,0x44,0x44,0x48,0x7F,0x00,0x00,0x00},
    ['e'] = {0x38,0x54,0x54,0x54,0x18,0x00,0x00,0x00},
    ['f'] = {0x08,0x7E,0x09,0x01,0x02,0x00,0x00,0x00},
    ['g'] = {0x0C,0x52,0x52,0x52,0x3E,0x00,0x00,0x00},
    ['h'] = {0x7F,0x08,0x04,0x04,0x78,0x00,0x00,0x00},
    ['i'] = {0x00,0x44,0x7D,0x40,0x00,0x00,0x00,0x00},
    ['j'] = {0x20,0x40,0x44,0x3D,0x00,0x00,0x00,0x00},
    ['k'] = {0x7F,0x10,0x28,0x44,0x00,0x00,0x00,0x00},
    ['l'] = {0x00,0x41,0x7F,0x40,0x00,0x00,0x00,0x00},
    ['m'] = {0x7C,0x04,0x18,0x04,0x78,0x00,0x00,0x00},
    ['n'] = {0x7C,0x08,0x04,0x04,0x78,0x00,0x00,0x00},
    ['o'] = {0x38,0x44,0x44,0x44,0x38,0x00,0x00,0x00},
    ['p'] = {0x7C,0x14,0x14,0x14,0x08,0x00,0x00,0x00},
    ['q'] = {0x08,0x14,0x14,0x18,0x7C,0x00,0x00,0x00},
    ['r'] = {0x7C,0x08,0x04,0x04,0x08,0x00,0x00,0x00},
    ['s'] = {0x48,0x54,0x54,0x54,0x20,0x00,0x00,0x00},
    ['t'] = {0x04,0x3F,0x44,0x40,0x20,0x00,0x00,0x00},
    ['u'] = {0x3C,0x40,0x40,0x20,0x7C,0x00,0x00,0x00},
    ['v'] = {0x1C,0x20,0x40,0x20,0x1C,0x00,0x00,0x00},
    ['w'] = {0x3C,0x40,0x30,0x40,0x3C,0x00,0x00,0x00},
    ['x'] = {0x44,0x28,0x10,0x28,0x44,0x00,0x00,0x00},
    ['y'] = {0x0C,0x50,0x50,0x50,0x3C,0x00,0x00,0x00},
    ['z'] = {0x44,0x64,0x54,0x4C,0x44,0x00,0x00,0x00},
    ['0'] = {0x3E,0x51,0x49,0x45,0x3E,0x00,0x00,0x00},
    ['1'] = {0x00,0x42,0x7F,0x40,0x00,0x00,0x00,0x00},
    ['2'] = {0x42,0x61,0x51,0x49,0x46,0x00,0x00,0x00},
    ['3'] = {0x21,0x41,0x45,0x4B,0x31,0x00,0x00,0x00},
    ['4'] = {0x18,0x14,0x12,0x7F,0x10,0x00,0x00,0x00},
    ['5'] = {0x27,0x45,0x45,0x45,0x39,0x00,0x00,0x00},
    ['6'] = {0x3C,0x4A,0x49,0x49,0x30,0x00,0x00,0x00},
    ['7'] = {0x01,0x71,0x09,0x05,0x03,0x00,0x00,0x00},
    ['8'] = {0x36,0x49,0x49,0x49,0x36,0x00,0x00,0x00},
    ['9'] = {0x06,0x49,0x49,0x29,0x1E,0x00,0x00,0x00},
    ['.'] = {0x00,0x60,0x60,0x00,0x00,0x00,0x00,0x00},
    [','] = {0x00,0x80,0x60,0x00,0x00,0x00,0x00,0x00},
    ['!'] = {0x00,0x00,0x5F,0x00,0x00,0x00,0x00,0x00},
    ['?'] = {0x02,0x01,0x51,0x09,0x06,0x00,0x00,0x00},
    ['-'] = {0x00,0x08,0x08,0x08,0x08,0x00,0x00,0x00},
    [' '] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
};

// Draw character
void draw_char(int x, int y, char c, uint8_t color) {
    uint8_t uc = (uint8_t)c;
    if (uc < 32 || uc > 127) uc = (uint8_t)' ';
    const uint8_t* bitmap = font_8x8[uc - 32];
    for (int row = 0; row < 8; row++) {
        uint8_t row_data = bitmap[row];
        for (int col = 0; col < 8; col++) {
            if (row_data & (0x80 >> col)) {
                draw_pixel(x + col, y + row, color);
            }
        }
    }
}

// Draw string
void draw_string(int x, int y, const char* str, uint8_t color) {
    int orig_x = x;
    while (*str) {
        if (*str == '\n') {
            y += 10;
            x = orig_x;
        } else {
            draw_char(x, y, *str, color);
            x += 8;
        }
        str++;
    }
}

// Swap back buffer to screen
void swap_buffers(void) {
    uint8_t* target = framebuffer ? framebuffer : (uint8_t*)0xA0000;
    uint32_t pixel_count = current_width * current_height;
    for (uint32_t i = 0; i < pixel_count; i++) {
        target[i] = back_buffer[i];
    }
}

void init_graphics(void) {
    set_video_mode(MODE_VGA_320x200);
    clear_screen(XP_BLACK);
    swap_buffers();
}

int vga_set_mode(vga_mode_t mode) {
    set_video_mode((video_mode_t)mode);
    return 1;
}

void vga_clear(uint8_t color) {
    clear_screen(color);
}

void vga_set_pixel(int x, int y, uint8_t color) {
    draw_pixel(x, y, color);
}

uint8_t vga_get_pixel(int x, int y) {
    if (x < 0 || y < 0 || x >= (int)current_width || y >= (int)current_height) {
        return 0;
    }
    return back_buffer[(y * (int)current_width) + x];
}

void vga_fill_rect(int x, int y, int w, int h, uint8_t color) {
    fill_rect(x, y, w, h, color);
}

void vga_draw_rect(int x, int y, int w, int h, uint8_t color) {
    draw_rect(x, y, w, h, color);
}

void vga_draw_line(int x1, int y1, int x2, int y2, uint8_t color) {
    draw_line(x1, y1, x2, y2, color);
}

void vga_draw_circle(int center_x, int center_y, int radius, uint8_t color) {
    int x = radius;
    int y = 0;
    int err = 0;

    while (x >= y) {
        draw_pixel(center_x + x, center_y + y, color);
        draw_pixel(center_x + y, center_y + x, color);
        draw_pixel(center_x - y, center_y + x, color);
        draw_pixel(center_x - x, center_y + y, color);
        draw_pixel(center_x - x, center_y - y, color);
        draw_pixel(center_x - y, center_y - x, color);
        draw_pixel(center_x + y, center_y - x, color);
        draw_pixel(center_x + x, center_y - y, color);

        y++;
        if (err <= 0) {
            err += 2 * y + 1;
        } else {
            x--;
            err -= 2 * x + 1;
        }
    }
}

void vga_fill_circle(int center_x, int center_y, int radius, uint8_t color) {
    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            if ((x * x) + (y * y) <= (radius * radius)) {
                draw_pixel(center_x + x, center_y + y, color);
            }
        }
    }
}

void vga_draw_string(int x, int y, const char* str, uint8_t color) {
    draw_string(x, y, str, color);
}

void vga_blit_buffer(uint32_t* src_buffer, uint32_t src_width, uint32_t src_height,
                     int dest_x, int dest_y, int width, int height) {
    if (!src_buffer || width <= 0 || height <= 0) return;

    if ((uint32_t)width > src_width) width = (int)src_width;
    if ((uint32_t)height > src_height) height = (int)src_height;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            uint8_t color = (uint8_t)src_buffer[(y * (int)src_width) + x];
            draw_pixel(dest_x + x, dest_y + y, color);
        }
    }
}

void vga_draw_bitmap_cursor(int x, int y) {
    static const uint8_t cursor_shape[12][12] = {
        {1,0,0,0,0,0,0,0,0,0,0,0},
        {1,1,0,0,0,0,0,0,0,0,0,0},
        {1,2,1,0,0,0,0,0,0,0,0,0},
        {1,2,2,1,0,0,0,0,0,0,0,0},
        {1,2,2,2,1,0,0,0,0,0,0,0},
        {1,2,2,2,2,1,0,0,0,0,0,0},
        {1,2,2,2,2,2,1,0,0,0,0,0},
        {1,2,2,2,2,2,2,1,0,0,0,0},
        {1,2,2,2,2,2,2,2,1,0,0,0},
        {1,2,2,2,2,2,1,1,1,0,0,0},
        {1,2,2,1,2,1,0,0,0,0,0,0},
        {1,1,1,0,1,1,0,0,0,0,0,0},
    };

    for (int row = 0; row < 12; row++) {
        for (int col = 0; col < 12; col++) {
            if (cursor_shape[row][col] == 1) draw_pixel(x + col, y + row, XP_WHITE);
            else if (cursor_shape[row][col] == 2) draw_pixel(x + col, y + row, XP_BLACK);
        }
    }
}

void vga_draw_shadow(int x, int y, uint32_t w, uint32_t h, uint32_t color, uint8_t offset) {
    fill_rect(x + offset, y + offset, (int)w, (int)h, (uint8_t)color);
}

void vga_fill_rounded_rect(int x, int y, uint32_t w, uint32_t h, uint8_t radius, uint32_t color) {
    (void)radius;
    fill_rect(x, y, (int)w, (int)h, (uint8_t)color);
}

void vga_draw_rounded_rect(int x, int y, uint32_t w, uint32_t h, uint8_t radius, uint32_t color) {
    (void)radius;
    draw_rect(x, y, (int)w, (int)h, (uint8_t)color);
}
