#ifndef USB_MOUSE_H
#define USB_MOUSE_H

#include "stdint.h"

// USB Mouse interface
void usb_mouse_init(void);
void usb_mouse_poll(void);
int usb_mouse_get_state(int32_t* x, int32_t* y, uint8_t* buttons);
int usb_mouse_available(void);

#endif
