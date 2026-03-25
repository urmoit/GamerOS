#ifndef FONT_H
#define FONT_H

#include "stdint.h"

// 8x8 bitmap font data for ASCII characters 32-127
// Each character is 8 bytes (8 rows of 8 bits)
extern const uint8_t font_8x8[96][8];

typedef enum {
    FONT_SIZE_8X8 = 8,
    FONT_SIZE_12X12 = 12,
    FONT_SIZE_16X16 = 16
} font_size_t;

void font_get_size(font_size_t size, uint8_t* out_w, uint8_t* out_h);
uint8_t font_sample_pixel(char c, font_size_t size, uint8_t x, uint8_t y);
uint32_t font_measure_text(const char* text, font_size_t size);

#endif // FONT_H
