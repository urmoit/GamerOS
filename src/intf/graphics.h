#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "stdint.h"

// VGA Mode 13h - 320x200x256 colors
#define VGA_WIDTH 320
#define VGA_HEIGHT 200
#define VGA_MODE13_SEGMENT 0xA000

// Initialize VGA graphics mode 13h
void vga_init_mode13(void);

// Set a pixel at (x, y) with color
void vga_set_pixel(uint32_t x, uint32_t y, uint8_t color);

// Get pixel color at (x, y)
uint8_t vga_get_pixel(uint32_t x, uint32_t y);

// Fill a rectangle with color
void vga_fill_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint8_t color);
void vga_draw_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint8_t color);
void vga_set_desktop_background(void);
void vga_clear(uint8_t color);

#endif



