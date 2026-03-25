#ifndef WINDOW_MANAGER_H
#define WINDOW_MANAGER_H

#include <stdint.h>
#include <window.h>

// Window manager functions
void window_manager_init(void);
void window_manager_shutdown(void);
uint32_t window_create(int x, int y, uint32_t width, uint32_t height, const char* title);
void window_destroy(uint32_t window_id);
void window_show(uint32_t window_id);
void window_hide(uint32_t window_id);
void window_move(uint32_t window_id, int x, int y);
void window_resize(uint32_t window_id, uint32_t width, uint32_t height);
void window_set_focus(uint32_t window_id);
void window_minimize(uint32_t window_id);
void window_maximize(uint32_t window_id);
void window_restore(uint32_t window_id);
void window_bring_to_front(uint32_t window_id);
void window_send_to_back(uint32_t window_id);

// Window enumeration
uint32_t window_get_count(void);
uint32_t window_get_active(void);
uint32_t window_get_at_position(int x, int y);

#endif