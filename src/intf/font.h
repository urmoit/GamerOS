#ifndef FONT_H
#define FONT_H

#include "stdint.h"

// 8x8 bitmap font data for ASCII characters 32-127
// Each character is 8 bytes (8 rows of 8 bits)
extern const uint8_t font_8x8[96][8];

#endif // FONT_H