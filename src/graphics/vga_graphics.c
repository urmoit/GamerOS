#include "../../intf/graphics.h"
#include "../../intf/ports.h"
#include "../../intf/font.h"
#include "../../intf/string.h"
#include "../../intf/multiboot.h"

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
    framebuffer_bpp = 8;
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
        framebuffer_bpp = 8;
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

uint8_t graphics_get_bpp(void) {
    if (framebuffer_truecolor && (framebuffer_bpp == 24 || framebuffer_bpp == 32)) {
        return framebuffer_bpp;
    }
    return 16;
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
    for (uint32_t i = 0; i < pixel_count; i++) {
        back_buffer_idx[i] = color;
        back_buffer_rgb[i] = rgb;
    }
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
    for (uint32_t i = 0; i < pixel_count; i++) {
        back_buffer_rgb[i] = color;
    }
}

// Fill rectangle
void fill_rect(int x, int y, int w, int h, uint8_t color) {
    for (int yy = y; yy < y + h && yy < (int)current_height; yy++) {
        for (int xx = x; xx < x + w && xx < (int)current_width; xx++) {
            if (xx >= 0 && yy >= 0) {
                back_buffer_idx[yy * current_width + xx] = color;
                back_buffer_rgb[(yy * current_width) + xx] = palette_rgb[color];
            }
        }
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
        for (uint32_t y = 0; y < dst_h; y++) {
            uint32_t sy = (y * current_height) / dst_h;
            uint32_t src_off = sy * current_width;
            uint8_t* row = (uint8_t*)target + ((uint32_t)y * framebuffer_pitch_bytes);
            if (framebuffer_bpp == 32) {
                uint32_t* row32 = (uint32_t*)row;
                for (uint32_t x = 0; x < dst_w; x++) {
                    uint32_t sx = (x * current_width) / dst_w;
                    row32[x] = back_buffer_rgb[src_off + sx];
                }
            } else {
                for (uint32_t x = 0; x < dst_w; x++) {
                    uint32_t sx = (x * current_width) / dst_w;
                    uint32_t rgb = back_buffer_rgb[src_off + sx];
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

void vga_fill_rounded_rect(int x, int y, uint32_t w, uint32_t h, uint8_t radius, uint32_t color) {
    (void)radius;
    fill_rect(x, y, (int)w, (int)h, (uint8_t)color);
}

void vga_draw_rounded_rect(int x, int y, uint32_t w, uint32_t h, uint8_t radius, uint32_t color) {
    (void)radius;
    draw_rect(x, y, (int)w, (int)h, (uint8_t)color);
}

// TODO: Add dirty-rectangle coalescing so multiple small updates batch into one planar present.
// TODO: Preserve/restore palette state across mode switches to avoid host-dependent color drift.
