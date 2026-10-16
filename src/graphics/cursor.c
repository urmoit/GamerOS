#include <graphics.h>

#define CURSOR_SIZE 12

// Arrow cursor bitmap: 1 = outline, 2 = fill, 0 = transparent
static const uint8_t cursor_shape[CURSOR_SIZE][CURSOR_SIZE] = {
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

void draw_cursor(int32_t x, int32_t y) {
    for (int row = 0; row < CURSOR_SIZE; row++) {
        for (int col = 0; col < CURSOR_SIZE; col++) {
            uint8_t pixel = cursor_shape[row][col];
            if (pixel == 1) {
                draw_pixel(x + col, y + row, XP_WHITE);
            } else if (pixel == 2) {
                draw_pixel(x + col, y + row, XP_BLACK);
            }
        }
    }
}
