#ifndef WINDOW_H
#define WINDOW_H

#include "stdint.h"

#define MAX_WINDOWS 10

typedef struct {
    int x, y, width, height;
    char* title;
    int is_active;
} window_t;

extern window_t* windows[MAX_WINDOWS];
extern int window_count;

void init_windowing();
window_t* create_window(int x, int y, int width, int height, char* title);
void draw_window(window_t* win);
void move_window(window_t* win, int new_x, int new_y);
void resize_window(window_t* win, int new_width, int new_height);
void set_window_focus(window_t* win);
void bring_window_to_front(window_t* win);
window_t* get_focused_window(void);

// TODO: Add window event handling (close, minimize, maximize)
// TODO: Implement window content clipping and scrolling

#endif
