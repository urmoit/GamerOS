#ifndef MOUSE_H
#define MOUSE_H

#include "stdint.h"

// Mouse button flags
#define MOUSE_LEFT_BUTTON 0x01
#define MOUSE_RIGHT_BUTTON 0x02
#define MOUSE_MIDDLE_BUTTON 0x04

// Mouse state structure
typedef struct {
    int32_t x;
    int32_t y;
    uint8_t buttons;  // Bit 0=left, 1=right, 2=middle
} mouse_state_t;

void mouse_init();
void mouse_handler();
mouse_state_t mouse_get_state();
int32_t mouse_get_x();
int32_t mouse_get_y();
uint8_t mouse_get_buttons();

#endif