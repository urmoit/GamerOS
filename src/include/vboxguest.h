#ifndef VBOXGUEST_H
#define VBOXGUEST_H

#include "stdint.h"

// VirtualBox VMM absolute mouse device.
void vboxguest_init(void);

int vboxguest_mouse_poll(void);

int vboxguest_present(void);

#endif
