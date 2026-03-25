#ifndef CURSOR_ARROW_H
#define CURSOR_ARROW_H

#include <stdint.h>

// Simple 16x16 cursor with 1-bit transparency (0=transparent, 1=visible)
// Using a simpler format that's easier to work with in VGA mode
#define CURSOR_WIDTH 16
#define CURSOR_HEIGHT 16
#define CURSOR_HOTSPOT_X 0
#define CURSOR_HOTSPOT_Y 0

// 16x16 mouse cursor bitmap (1 bit per pixel, 0=transparent, 1=pixels)
// Stored as 16 rows of 16 bits each
// This creates a classic Windows-style arrow cursor
static const uint16_t cursor_arrow_bitmap[CURSOR_HEIGHT] = {
    0b1000000000000000,  // Row 0:  |
    0b1100000000000000,  // Row 1:  ||
    0b1110000000000000,  // Row 2:  |||
    0b1111000000000000,  // Row 3:  ||||
    0b1111100000000000,  // Row 4:  |||||
    0b1111110000000000,  // Row 5:  ||||||
    0b1111111000000000,  // Row 6:  |||||||
    0b1111111100000000,  // Row 7:  ||||||||
    0b1111111110000000,  // Row 8:  |||||||||
    0b1111111111000000,  // Row 9:  ||||||||||
    0b1111110000000000,  // Row 10: ||||
    0b1110110000000000,  // Row 11: || ||
    0b1100110000000000,  // Row 12: ||  ||
    0b1000011000000000,  // Row 13: |   ||
    0b0000011000000000,  // Row 14:     ||
    0b0000001100000000   // Row 15:      ||
};

// Cursor colors (for VGA mode 13h palette indices)
#define CURSOR_OUTLINE_COLOR 0x00  // Black outline
#define CURSOR_FILL_COLOR 0x0F     // White fill

#endif
