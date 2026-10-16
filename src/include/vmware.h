#ifndef VMWARE_H
#define VMWARE_H

#include "stdint.h"

// VMware backdoor absolute pointer protocol.
#define VMWARE_MAGIC 0x564D5868
#define VMWARE_PORT  0x5658

#define CMD_GET_VERSION              10
#define CMD_ABS_POINTER_COMMAND      41
#define CMD_ABS_POINTER_STATUS       40
#define CMD_ABS_POINTER_DATA         39

// Sub-commands of CMD_ABS_POINTER_COMMAND.
#define ABS_POINTER_ENABLE   0x45414552
#define ABS_POINTER_RELATIVE 0xF5
#define ABS_POINTER_ABSOLUTE 0x53424152

#define ABS_STATUS_RELATIVE  0x00010000
#define ABS_STATUS_BUTTON_LEFT   0x20
#define ABS_STATUS_BUTTON_RIGHT  0x10
#define ABS_STATUS_BUTTON_MIDDLE 0x08

void vmware_mouse_init(void);

int vmware_mouse_poll(void);

int vmware_mouse_active(void);

#endif
