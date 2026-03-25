#ifndef MOUSE_H
#define MOUSE_H

#include "stdint.h"

#define CURSOR_SIZE 12

#define MOUSE_BTN_LEFT    0x01
#define MOUSE_BTN_RIGHT   0x02
#define MOUSE_BTN_MIDDLE  0x04

typedef struct {
    int32_t x;
    int32_t y;
    uint8_t buttons;
} mouse_state_t;

void mouse_init(void);
void mouse_send_cmd(uint8_t cmd);
void mouse_handler(void);
void mouse_poll(void);

int32_t mouse_get_x(void);
int32_t mouse_get_y(void);
uint8_t mouse_get_buttons(void);
mouse_state_t mouse_get_state(void);
int8_t mouse_get_wheel_delta(void);
void mouse_clear_buttons(void);

void draw_cursor(int32_t x, int32_t y);

#endif
