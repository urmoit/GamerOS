#ifndef XHCI_H
#define XHCI_H

#include "stdint.h"

// xHCI (USB 3.0) host controller driver.
void xhci_init(void);

void xhci_poll(void);

int xhci_keyboard_connected(void);
int xhci_mouse_connected(void);

#endif
