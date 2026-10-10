#include <graphics.h>
#include <ports.h>
#include <font.h>
#include <string.h>
#include <multiboot.h>

// Current mode settings
uint32_t current_width = 640;
uint32_t current_height = 480;
uint8_t* framebuffer = (uint8_t*)0xA0000;
int graphics_mode = 1;

// Back buffers (max 1920x1080). Keep indexed and RGB paths for compatibility.
#define MAX_RENDER_WIDTH 1920
#define MAX_RENDER_HEIGHT 1080
#define MAX_RENDER_PIXELS (MAX_RENDER_WIDTH * MAX_RENDER_HEIGHT)
static uint8_t back_buffer_idx[MAX_RENDER_PIXELS];
static uint32_t back_buffer_rgb[MAX_RENDER_PIXELS];
static uint32_t framebuffer_pitch_bytes = 0;
static uint8_t framebuffer_bpp = 0;
static uint8_t framebuffer_truecolor = 0;
static uint32_t native_width = 640;
static uint32_t native_height = 480;

static uint32_t palette_rgb[256];
static uint8_t palette_ready = 0;

static inline uint8_t pal6_to_8(uint8_t v6) {
    return (uint8_t)((v6 * 255U) / 63U);
}

static void palette_set_entry(uint8_t idx, uint8_t r6, uint8_t g6, uint8_t b6) {
    palette_rgb[idx] = ((uint32_t)pal6_to_8(r6) << 16) |
                       ((uint32_t)pal6_to_8(g6) << 8) |
                       ((uint32_t)pal6_to_8(b6));
}

static void palette_init_defaults(void) {
    for (int i = 0; i < 256; i++) {
        uint8_t c = (uint8_t)i;
        palette_rgb[i] = ((uint32_t)c << 16) | ((uint32_t)c << 8) | c;
    }
}

static void palette_apply_shell_entries(void) {
    static const uint8_t pal16[16][3] = {
        {0,0,0}, {0,0,42}, {0,42,0}, {0,42,42},
        {42,0,0}, {42,0,42}, {42,21,0}, {42,42,42},
        {21,21,21}, {21,21,63}, {21,63,21}, {21,63,63},
        {63,21,21}, {63,21,63}, {63,63,21}, {63,63,63}
    };
    for (uint8_t i = 0; i < 16; i++) {
        palette_set_entry(i, pal16[i][0], pal16[i][1], pal16[i][2]);
    }
    // Custom shell palette indices used by desktop/theme assets.
    palette_set_entry(0x39, 36, 50, 58);
    palette_set_entry(0x2A, 38, 48, 12);
    palette_set_entry(0x3D, 56, 46, 18);
    palette_set_entry(0x3E, 16, 24, 30);
}

static void ensure_palette_ready(void) {
    if (palette_ready) return;
    palette_init_defaults();
    palette_apply_shell_entries();
    palette_ready = 1;
}

static inline void vga_write_seq(uint8_t index, uint8_t value) {
    outb(0x3C4, index);
    outb(0x3C5, value);
}

static inline void vga_write_crtc(uint8_t index, uint8_t value) {
    outb(0x3D4, index);
    outb(0x3D5, value);
}

static inline void vga_write_gc(uint8_t index, uint8_t value) {
    outb(0x3CE, index);
    outb(0x3CF, value);
}

static inline void vga_write_ac(uint8_t index, uint8_t value) {
    (void)inb(0x3DA); // Reset AC flip-flop before each indexed write.
    outb(0x3C0, index);
    outb(0x3C0, value);
}

static void vga_set_mode_12h_runtime(void) {
    static const uint8_t crtc_data_12h[25] = {
        0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0x0B, 0x3E,
        0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xEA, 0x8C, 0xDF, 0x28, 0x00, 0xE7, 0x04, 0xC3,
        0xFF
    };

    outb(0x3C2, 0xE3);

    // Sequencer reset while programming mode registers.
    vga_write_seq(0x00, 0x01);
    vga_write_seq(0x01, 0x01);
    vga_write_seq(0x02, 0x0F);
    vga_write_seq(0x03, 0x00);
    vga_write_seq(0x04, 0x06);

    // Unprotect CRTC and load full 640x480 timing set.
    outb(0x3D4, 0x11);
    outb(0x3D5, (uint8_t)(inb(0x3D5) & 0x7F));
    for (uint8_t i = 0; i < 25; i++) {
        vga_write_crtc(i, crtc_data_12h[i]);
    }

    // Graphics controller for 16-color planar framebuffer at A0000.
    vga_write_gc(0x00, 0x00);
    vga_write_gc(0x01, 0x00);
    vga_write_gc(0x02, 0x00);
    vga_write_gc(0x03, 0x00);
    vga_write_gc(0x04, 0x00);
    vga_write_gc(0x05, 0x00);
    vga_write_gc(0x06, 0x05);
    vga_write_gc(0x07, 0x00);
    vga_write_gc(0x08, 0xFF);

    // Attribute controller palette/control.
    for (uint8_t i = 0; i < 16; i++) {
        vga_write_ac(i, i);
    }
    vga_write_ac(0x10, 0x01); // Graphics mode, 16-color path.
    vga_write_ac(0x11, 0x00);
    vga_write_ac(0x12, 0x0F);
    vga_write_ac(0x13, 0x00);
    vga_write_ac(0x14, 0x00);

    // Re-enable display and sequencer.
    (void)inb(0x3DA);
    outb(0x3C0, 0x20);
    vga_write_seq(0x00, 0x03);

    // Clear visible planes.
    vga_write_gc(0x05, 0x00);
    vga_write_gc(0x06, 0x05);
    vga_write_gc(0x08, 0xFF);
    vga_write_seq(0x02, 0x0F);
    volatile uint8_t* vram = (volatile uint8_t*)0xA0000;
    for (int i = 0; i < (80 * 480); i++) {
        vram[i] = 0x00;
    }
}

static void vga_prepare_mode12_planar_writes(void) {
    // Sequencer: disable chain-4 and enable planar memory mode.
    outb(0x3C4, 0x04);
    outb(0x3C5, 0x06);

    // Graphics controller: deterministic write mode 0 state for planar memory writes.
    outb(0x3CE, 0x00); // Set/Reset
    outb(0x3CF, 0x00);
    outb(0x3CE, 0x01); // Enable Set/Reset
    outb(0x3CF, 0x00);
    outb(0x3CE, 0x03); // Data Rotate
    outb(0x3CF, 0x00);
    outb(0x3CE, 0x04); // Read Map Select
    outb(0x3CF, 0x00);
    outb(0x3CE, 0x05);
    outb(0x3CF, 0x00);
    outb(0x3CE, 0x06);
    outb(0x3CF, 0x05);
    outb(0x3CE, 0x08);
    outb(0x3CF, 0xFF);
}

// VGA Palette setup
void vga_set_palette(void) {
    palette_init_defaults();
    palette_apply_shell_entries();
    palette_ready = 1;

    for (uint8_t i = 0; i < 16; i++) {
        outb(0x3C8, i);
        // Keep VGA DAC in sync with software palette mapping.
        uint32_t rgb = palette_rgb[i];
        uint8_t r6 = (uint8_t)(((rgb >> 16) & 0xFF) * 63 / 255);
        uint8_t g6 = (uint8_t)(((rgb >> 8) & 0xFF) * 63 / 255);
        uint8_t b6 = (uint8_t)((rgb & 0xFF) * 63 / 255);
        outb(0x3C9, r6);
        outb(0x3C9, g6);
        outb(0x3C9, b6);
    }

    // Preserve custom shell indices used by UI assets/icons.
    outb(0x3C8, 0x39);
    outb(0x3C9, 36);
    outb(0x3C9, 50);
    outb(0x3C9, 58);

    outb(0x3C8, 0x2A);
    outb(0x3C9, 38);
    outb(0x3C9, 48);
    outb(0x3C9, 12);

    outb(0x3C8, 0x3D);
    outb(0x3C9, 56);
    outb(0x3C9, 46);
    outb(0x3C9, 18);

    outb(0x3C8, 0x3E);
    outb(0x3C9, 16);
    outb(0x3C9, 24);
    outb(0x3C9, 30);
}

// Set standard VGA 320x200 mode
void vga_set_mode_13h(void) {
    // BIOS interrupts are not valid in long mode. Program VGA mode 12h directly.
    vga_set_mode_12h_runtime();
    current_width = 640;
    current_height = 480;
    framebuffer = (uint8_t*)0xA0000;
    graphics_mode = 1;
    framebuffer_pitch_bytes = current_width;
    framebuffer_bpp = 4;
    framebuffer_truecolor = 0;
    native_width = current_width;
    native_height = current_height;

    vga_prepare_mode12_planar_writes();
    vga_set_palette();
}

// Set VESA mode
int vesa_set_mode(uint16_t mode) {
    // Reuse pre-set VGA mode 12h as the high-res runtime mode in long mode.
    if (mode == 0x101) {
        vga_set_mode_12h_runtime();
        current_width = 640;
        current_height = 480;
        framebuffer = (uint8_t*)0xA0000;
        graphics_mode = 1;
        framebuffer_pitch_bytes = current_width;
        framebuffer_bpp = 4;
        framebuffer_truecolor = 0;
        native_width = current_width;
        native_height = current_height;
        vga_prepare_mode12_planar_writes();
        vga_set_palette();
        return 1;
    }
    return 0;
}

int graphics_use_multiboot_framebuffer(const struct multiboot_info* mb_info) {
    ensure_palette_ready();
    if (!mb_info) return 0;
    if (!(mb_info->flags & MULTIBOOT_FLAG_FB)) return 0;
    if (mb_info->framebuffer_addr == 0) return 0;
    if (mb_info->framebuffer_type != 1) return 0; // Require RGB framebuffer.
    if (mb_info->framebuffer_width == 0 || mb_info->framebuffer_height == 0) return 0;
    if (mb_info->framebuffer_width > MAX_RENDER_WIDTH || mb_info->framebuffer_height > MAX_RENDER_HEIGHT) return 0;
    if (!(mb_info->framebuffer_bpp == 24 || mb_info->framebuffer_bpp == 32)) return 0;
    if (mb_info->framebuffer_pitch == 0) return 0;

    uint32_t bytes_per_pixel = (uint32_t)(mb_info->framebuffer_bpp / 8);
    uint32_t min_pitch = mb_info->framebuffer_width * bytes_per_pixel;
    if (mb_info->framebuffer_pitch < min_pitch) return 0;

    // Kernel currently identity-maps only the first 4GB of physical memory.
    // Reject framebuffers that are outside this range to avoid page faults/triple faults.
    uint64_t fb_addr = mb_info->framebuffer_addr;
    uint64_t fb_size = (uint64_t)mb_info->framebuffer_pitch * (uint64_t)mb_info->framebuffer_height;
    if (fb_addr > 0xFFFFFFFFULL) return 0;
    if (fb_size == 0) return 0;
    if ((fb_addr + fb_size) > 0x100000000ULL) return 0;

    framebuffer = (uint8_t*)(uintptr_t)fb_addr;
    current_width = mb_info->framebuffer_width;
    current_height = mb_info->framebuffer_height;
    framebuffer_pitch_bytes = mb_info->framebuffer_pitch;
    framebuffer_bpp = mb_info->framebuffer_bpp;
    framebuffer_truecolor = 1;
    graphics_mode = 2;
    native_width = current_width;
    native_height = current_height;
    return 1;
}

int graphics_is_truecolor(void) {
    return framebuffer_truecolor ? 1 : 0;
}

uint32_t graphics_get_pixel_rgb(int x, int y) {
    if (x < 0 || y < 0 || x >= (int)current_width || y >= (int)current_height) return 0;
    return back_buffer_rgb[(y * (int)current_width) + x] & 0x00FFFFFF;
}

// Direct access to one back-buffer scanline of the RGB path.
// Callers must not retain the pointer past a resolution change.
uint32_t* graphics_get_rgb_row(int y) {
    if (y < 0 || y >= (int)current_height) return 0;
    return &back_buffer_rgb[(uint32_t)y * current_width];
}

uint8_t graphics_get_bpp(void) {
    if (framebuffer_truecolor && (framebuffer_bpp == 24 || framebuffer_bpp == 32)) {
        return framebuffer_bpp;
    }
    // The indexed fallback path programs VGA mode 12h: 640x480x16 planar,
    // i.e. 4 bits per pixel, not 16.
    return 4;
}

uint32_t graphics_get_native_width(void) {
    return native_width;
}

uint32_t graphics_get_native_height(void) {
    return native_height;
}

int graphics_set_resolution(uint32_t width, uint32_t height) {
    if (width == 0 || height == 0) return 0;
    if (width > MAX_RENDER_WIDTH || height > MAX_RENDER_HEIGHT) return 0;

    if (!framebuffer_truecolor) {
        if (width != 640 || height != 480) return 0;
        current_width = 640;
        current_height = 480;
        native_width = 640;
        native_height = 480;
        return 1;
    }

    if (width > native_width || height > native_height) return 0;
    current_width = width;
    current_height = height;
    clear_screen(0);
    swap_buffers();
    return 1;
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
        case MODE_VESA_1920x1080:
            // True 1080p is provided via multiboot framebuffer path.
            // Keep this mode token as an explicit fallback target.
            if (!vesa_set_mode(0x101)) {
                vga_set_mode_13h();
            }
            break;
    }
}

// Clear back buffer
void clear_screen(uint8_t color) {
    ensure_palette_ready();
    uint32_t pixel_count = current_width * current_height;
    uint32_t rgb = palette_rgb[color];
    memset(back_buffer_idx, color, pixel_count);
    uint32_t* dst = back_buffer_rgb;
    uint32_t count = pixel_count;
    while (count >= 8) {
        dst[0] = rgb; dst[1] = rgb; dst[2] = rgb; dst[3] = rgb;
        dst[4] = rgb; dst[5] = rgb; dst[6] = rgb; dst[7] = rgb;
        dst += 8; count -= 8;
    }
    for (uint32_t i = 0; i < count; i++) dst[i] = rgb;
}

// Draw pixel to back buffer
void draw_pixel(int x, int y, uint8_t color) {
    ensure_palette_ready();
    if (x < 0 || x >= (int)current_width || y < 0 || y >= (int)current_height) return;
    uint32_t off = ((uint32_t)y * current_width) + (uint32_t)x;
    back_buffer_idx[off] = color;
    back_buffer_rgb[off] = palette_rgb[color];
}

void draw_pixel_rgb(int x, int y, uint32_t rgb) {
    if (x < 0 || x >= (int)current_width || y < 0 || y >= (int)current_height) return;
    uint32_t off = ((uint32_t)y * current_width) + (uint32_t)x;
    back_buffer_rgb[off] = rgb & 0x00FFFFFF;
}

void clear_screen_rgb(uint32_t rgb) {
    uint32_t pixel_count = current_width * current_height;
    uint32_t color = rgb & 0x00FFFFFF;
    uint32_t* dst = back_buffer_rgb;
    uint32_t count = pixel_count;
    while (count >= 8) {
        dst[0] = color; dst[1] = color; dst[2] = color; dst[3] = color;
        dst[4] = color; dst[5] = color; dst[6] = color; dst[7] = color;
        dst += 8; count -= 8;
    }
    for (uint32_t i = 0; i < count; i++) dst[i] = color;
}

// Fill rectangle
void fill_rect(int x, int y, int w, int h, uint8_t color) {
    if (w <= 0 || h <= 0) return;
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x >= (int)current_width || y >= (int)current_height) return;
    if (x + w > (int)current_width) w = (int)current_width - x;
    if (y + h > (int)current_height) h = (int)current_height - y;
    if (w <= 0 || h <= 0) return;

    uint32_t rgb = palette_rgb[color];
    uint32_t row_stride = current_width;
    uint32_t base_off = (uint32_t)y * row_stride + (uint32_t)x;

    for (int yy = 0; yy < h; yy++) {
        uint32_t off = base_off + (uint32_t)yy * row_stride;
        memset(&back_buffer_idx[off], color, (size_t)w);
        uint32_t* dst = &back_buffer_rgb[off];
        int cw = w;
        while (cw >= 8) {
            dst[0] = rgb; dst[1] = rgb; dst[2] = rgb; dst[3] = rgb;
            dst[4] = rgb; dst[5] = rgb; dst[6] = rgb; dst[7] = rgb;
            dst += 8; cw -= 8;
        }
        for (int i = 0; i < cw; i++) dst[i] = rgb;
    }
}

// Draw rectangle outline
void draw_rect(int x, int y, int w, int h, uint8_t color) {
    for (int xx = x; xx < x + w && xx < (int)current_width; xx++) {
        if (xx >= 0 && y >= 0 && y < (int)current_height) {
            back_buffer_idx[y * current_width + xx] = color;
            back_buffer_rgb[(y * current_width) + xx] = palette_rgb[color];
        }
        if (xx >= 0 && y + h - 1 >= 0 && y + h - 1 < (int)current_height) {
            back_buffer_idx[(y + h - 1) * current_width + xx] = color;
            back_buffer_rgb[((y + h - 1) * current_width) + xx] = palette_rgb[color];
        }
    }
    for (int yy = y; yy < y + h && yy < (int)current_height; yy++) {
        if (x >= 0 && x < (int)current_width && yy >= 0) {
            back_buffer_idx[yy * current_width + x] = color;
            back_buffer_rgb[(yy * current_width) + x] = palette_rgb[color];
        }
        if (x + w - 1 >= 0 && x + w - 1 < (int)current_width && yy >= 0) {
            back_buffer_idx[yy * current_width + (x + w - 1)] = color;
            back_buffer_rgb[(yy * current_width) + (x + w - 1)] = palette_rgb[color];
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
    volatile uint8_t* target = framebuffer ? (volatile uint8_t*)framebuffer : (volatile uint8_t*)0xA0000;
    if (graphics_mode == 1 && current_width == 640 && current_height == 480) {
        static uint8_t plane_buffer[4][80 * 480];
        memset(plane_buffer, 0, sizeof(plane_buffer));

        for (int y = 0; y < 480; y++) {
            for (int x = 0; x < 640; x++) {
                uint8_t color = back_buffer_idx[(y * 640) + x] & 0x0F;
                int offset = (y * 80) + (x >> 3);
                uint8_t bit = (uint8_t)(0x80 >> (x & 7));
                if (color & 0x01) plane_buffer[0][offset] |= bit;
                if (color & 0x02) plane_buffer[1][offset] |= bit;
                if (color & 0x04) plane_buffer[2][offset] |= bit;
                if (color & 0x08) plane_buffer[3][offset] |= bit;
            }
        }

        vga_prepare_mode12_planar_writes();
        for (int plane = 0; plane < 4; plane++) {
            outb(0x3CE, 0x04);
            outb(0x3CF, (uint8_t)plane);
            outb(0x3CE, 0x08);
            outb(0x3CF, 0xFF);
            outb(0x3C4, 0x02);
            outb(0x3C5, (uint8_t)(1 << plane));
            for (int i = 0; i < (80 * 480); i++) {
                target[i] = plane_buffer[plane][i];
            }
        }
        outb(0x3C4, 0x02);
        outb(0x3C5, 0x0F);
    } else if (framebuffer_truecolor && (framebuffer_bpp == 32 || framebuffer_bpp == 24)) {
        uint32_t dst_w = native_width;
        uint32_t dst_h = native_height;
        if (dst_w == 0 || dst_h == 0) {
            dst_w = current_width;
            dst_h = current_height;
        }
        if (current_width == 0 || current_height == 0) return;

        if (framebuffer_bpp == 32) {
            uint32_t* fb32 = (uint32_t*)(uintptr_t)framebuffer;
            for (uint32_t y = 0; y < dst_h; y++) {
                uint32_t sy = (y * current_height) / dst_h;
                uint32_t src_off = sy * current_width;
                uint32_t* row32 = fb32 + ((uint32_t)y * (framebuffer_pitch_bytes / 4));
                // The logical row may be narrower than the native row, so scale
                // horizontally instead of assuming a 1:1 width.
                if (current_width == dst_w) {
                    memcpy(row32, &back_buffer_rgb[src_off], (size_t)dst_w * 4);
                } else {
                    for (uint32_t x = 0; x < dst_w; x++) {
                        uint32_t sx = (x * current_width) / dst_w;
                        row32[x] = back_buffer_rgb[src_off + sx];
                    }
                }
            }
        } else {
            for (uint32_t y = 0; y < dst_h; y++) {
                uint32_t sy = (y * current_height) / dst_h;
                uint32_t src_off = sy * current_width;
                uint8_t* row = (uint8_t*)target + ((uint32_t)y * framebuffer_pitch_bytes);
                uint32_t* src32 = &back_buffer_rgb[src_off];
                for (uint32_t x = 0; x < dst_w; x++) {
                    uint32_t sx = (current_width == dst_w) ? x : (x * current_width) / dst_w;
                    uint32_t rgb = src32[sx];
                    uint8_t* px = row + (x * 3);
                    px[0] = (uint8_t)(rgb & 0xFF);
                    px[1] = (uint8_t)((rgb >> 8) & 0xFF);
                    px[2] = (uint8_t)((rgb >> 16) & 0xFF);
                }
            }
        }
    } else {
        uint32_t pixel_count = current_width * current_height;
        for (uint32_t i = 0; i < pixel_count; i++) {
            target[i] = back_buffer_idx[i];
        }
    }
}

void present_rect(int x, int y, int w, int h) {
    if (w <= 0 || h <= 0) return;

    int x0 = x;
    int y0 = y;
    int x1 = x + w;
    int y1 = y + h;

    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > (int)current_width) x1 = (int)current_width;
    if (y1 > (int)current_height) y1 = (int)current_height;
    if (x0 >= x1 || y0 >= y1) return;

    volatile uint8_t* target = framebuffer ? (volatile uint8_t*)framebuffer : (volatile uint8_t*)0xA0000;

    if (graphics_mode == 1 && current_width == 640 && current_height == 480) {
        int byte_start = x0 >> 3;
        int byte_end = (x1 - 1) >> 3;

        vga_prepare_mode12_planar_writes();
        for (int plane = 0; plane < 4; plane++) {
            outb(0x3CE, 0x04);
            outb(0x3CF, (uint8_t)plane);
            outb(0x3CE, 0x08);
            outb(0x3CF, 0xFF);
            outb(0x3C4, 0x02);
            outb(0x3C5, (uint8_t)(1 << plane));

            for (int yy = y0; yy < y1; yy++) {
                int row_base = yy * 80;
                int src_row = yy * 640;
                for (int b = byte_start; b <= byte_end; b++) {
                    int px = b << 3;
                    uint8_t out = 0;
                    for (int bit = 0; bit < 8; bit++) {
                        int sx = px + bit;
                        if (sx < 0 || sx >= 640) continue;
                        uint8_t color = back_buffer_idx[src_row + sx] & 0x0F;
                        if (color & (1 << plane)) {
                            out |= (uint8_t)(0x80 >> bit);
                        }
                    }
                    target[row_base + b] = out;
                }
            }
        }
        outb(0x3C4, 0x02);
        outb(0x3C5, 0x0F);
    } else if (framebuffer_truecolor && (framebuffer_bpp == 32 || framebuffer_bpp == 24)) {
        // Scaled truecolor presentation uses whole-frame blits for correctness.
        swap_buffers();
    } else {
        for (int yy = y0; yy < y1; yy++) {
            uint32_t row_off = (uint32_t)yy * current_width;
            for (int xx = x0; xx < x1; xx++) {
                target[row_off + (uint32_t)xx] = back_buffer_idx[row_off + (uint32_t)xx];
            }
        }
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
    return back_buffer_idx[(y * (int)current_width) + x];
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

// ---------------------------------------------------------------------------
// Rounded rectangles
//
// A rounded rectangle is a rectangle whose four corners are quarter circles of
// `radius`. Only the corner squares need the circle test; every other pixel is a
// plain span, so each row is computed once as a left/right inset and then filled
// as a run. That keeps this cheap enough to run on every repaint.
// ---------------------------------------------------------------------------

// Integer square root. Only ever called with small values (radius^2), so a
// simple incremental scan is cheaper than any floating-point setup.
static int rr_isqrt(uint32_t v) {
    int r = 0;
    while ((uint32_t)((r + 1) * (r + 1)) <= v) r++;
    return r;
}

// How far the filled span is pulled in from the left edge on row `yy` of a
// rounded rect `w` x `h`. Zero on rows that clear the corner circles.
static int rr_row_inset(int w, int h, int radius, int yy) {
    if (radius <= 0) return 0;
    int r = radius;
    if (r * 2 > w) r = w / 2;
    if (r * 2 > h) r = h / 2;
    if (r <= 0) return 0;
    if (yy >= r && yy < h - r) return 0;

    // Distance from the corner arc's centre to this row.
    int dy = (yy < r) ? (r - yy) : (yy - (h - 1 - r));
    int half = rr_isqrt((uint32_t)((r * r) - (dy * dy)));
    int inset = r - half;
    return (inset < 0) ? 0 : inset;
}

void vga_fill_rounded_rect(int x, int y, uint32_t w, uint32_t h, uint8_t radius, uint32_t color) {
    if (w == 0 || h == 0) return;
    if (x < 0) {
        uint32_t cut = (uint32_t)(-x);
        if (cut >= w) return;
        w -= cut;
        x = 0;
    }
    if (y < 0) {
        uint32_t cut = (uint32_t)(-y);
        if (cut >= h) return;
        h -= cut;
        y = 0;
    }
    if (x >= (int)current_width || y >= (int)current_height) return;
    if (x + (int)w > (int)current_width) w = (uint32_t)((int)current_width - x);
    if (y + (int)h > (int)current_height) h = (uint32_t)((int)current_height - y);
    if (w == 0 || h == 0) return;

    uint8_t idx = (uint8_t)color;
    uint32_t rgb = palette_rgb[idx];
    int width = (int)w;
    int height = (int)h;

    for (int yy = 0; yy < height; yy++) {
        int inset = rr_row_inset(width, height, (int)radius, yy);
        int row = y + yy;
        uint32_t base = (uint32_t)row * current_width + (uint32_t)(x + inset);
        int run = width - (inset * 2);
        if (run <= 0) continue;
        memset(&back_buffer_idx[base], idx, (size_t)run);
        uint32_t* dst = &back_buffer_rgb[base];
        int cw = run;
        while (cw >= 8) {
            dst[0] = rgb; dst[1] = rgb; dst[2] = rgb; dst[3] = rgb;
            dst[4] = rgb; dst[5] = rgb; dst[6] = rgb; dst[7] = rgb;
            dst += 8; cw -= 8;
        }
        for (int i = 0; i < cw; i++) dst[i] = rgb;
    }
}

void vga_draw_rounded_rect(int x, int y, uint32_t w, uint32_t h, uint8_t radius, uint32_t color) {
    // A degenerate box has no interior to leave alone, so outline it as a fill.
    if (w < 3 || h < 3) {
        vga_fill_rounded_rect(x, y, w, h, radius, color);
        return;
    }
    if (x >= (int)current_width || y >= (int)current_height) return;

    uint8_t idx = (uint8_t)color;
    uint32_t rgb = palette_rgb[idx];
    int width = (int)w;
    int height = (int)h;
    int r = (int)radius;
    if (r * 2 > width) r = width / 2;
    if (r * 2 > height) r = height / 2;
    int inner_r = (r > 0) ? (r - 1) : 0;

    for (int yy = 0; yy < height; yy++) {
        int outer_in = rr_row_inset(width, height, r, yy);
        // The interior is the same shape inset by one pixel on every side.
        int inner_in = 0;
        if (yy >= 1 && yy <= height - 2) {
            inner_in = rr_row_inset(width - 2, height - 2, inner_r, yy - 1);
        }
        int outer_l = x + outer_in;
        int outer_r = x + width - 1 - outer_in;
        int inner_l = x + 1 + inner_in;
        int inner_r = x + width - 2 - inner_in;

        int row = y + yy;
        if (row < 0 || row >= (int)current_height) continue;
        for (int px = outer_l; px <= outer_r; px++) {
            if (px >= 0 && px < (int)current_width && (px < inner_l || px > inner_r)) {
                back_buffer_idx[(uint32_t)row * current_width + (uint32_t)px] = idx;
                back_buffer_rgb[(uint32_t)row * current_width + (uint32_t)px] = rgb;
            }
        }
    }
}

// TODO: Add dirty-rectangle coalescing so multiple small updates batch into one planar present.
// TODO: Preserve/restore palette state across mode switches to avoid host-dependent color drift.
