#ifndef VBOXGUEST_H
#define VBOXGUEST_H

#include "stdint.h"

// VirtualBox VMM device: absolute mouse (mouse integration).
//
// VirtualBox exposes its guest interface as a PCI device with vendor 0x80EE and
// device ID 0xCAFE. BAR0 is the I/O port a request is submitted through: the
// guest writes the *physical* address of a request block to it, and the block
// comes back filled in. Requests are synchronous, so a polling guest never needs
// the device interrupt.
//
// Using it removes the pointer capture from a VirtualBox window: instead of
// relative packets, the host reports an absolute position on a 0..0xFFFF grid
// that is scaled onto the desktop here.
void vboxguest_init(void);

// Consume one mouse position report. Returns 1 when the VMM device is present
// and the host currently wants absolute coordinates (mouse integration on).
// Buttons keep arriving over PS/2 in VirtualBox, so they are not touched here.
int vboxguest_mouse_poll(void);

int vboxguest_present(void);

#endif
