#ifndef CURSOR_H
#define CURSOR_H

#include "stdint.h"

// Cursor management functions
void cursor_draw(int32_t x, int32_t y);
void cursor_hide(void);
void cursor_update(int32_t new_x, int32_t new_y);
uint8_t cursor_is_visible(void);

#endif
