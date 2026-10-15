#ifndef XHCI_H
#define XHCI_H

#include "stdint.h"

// xHCI (eXtensible Host Controller Interface) driver.
//
// USB 3.0 controllers use a command/transfer/event ring model rather than the
// UHCI frame list: a command TRB is posted and the matching event TRB is
// waited for, and transfers queue on per-endpoint rings that are patted awake
// with a doorbell write. The event ring is polled rather than interrupt-driven
// because the desktop loop runs with interrupts off.
//
// This is the controller a plain QEMU machine exposes USB keyboards and mice
// on, through -device qemu-xhci. UHCI covers the 1.1 controllers, and the two
// drivers report into the same input state.

// Bring up every xHCI controller and enumerate the HID devices on its ports.
void xhci_init(void);

// Poll the event rings and consume any reports that arrived.
void xhci_poll(void);

int xhci_keyboard_connected(void);
int xhci_mouse_connected(void);

#endif
