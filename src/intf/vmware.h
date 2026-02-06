#ifndef VMWARE_H
#define VMWARE_H

#include "stdint.h"

// VMware Magic Numbers
#define VMWARE_MAGIC 0x564D5868
#define VMWARE_PORT  0x5658

// Backdoor Commands
#define CMD_GET_VERSION             10
#define CMD_GET_MOUSE_STATUS        4
#define CMD_SET_MOUSE_STATUS        5
#define CMD_GET_MOUSE_DATA          6
#define CMD_ABS_POINTER_DATA        39
#define CMD_ABS_POINTER_STATUS      40
#define CMD_ABS_POINTER_COMMAND     41

// Command parameters
#define ABS_POINTER_ENABLE          0x45414552
#define ABS_POINTER_RELATIVE        0xF5
#define ABS_POINTER_ABSOLUTE        0x53424152

// Data structures
typedef struct {
    int32_t x;
    int32_t y;
    uint32_t buttons;
    int32_t scroll;
} vmware_mouse_data_t;

// Functions
int vmware_detect(void);
void vmware_enable_mouse(void);
int vmware_get_mouse_data(vmware_mouse_data_t* data);
void vmware_disable_mouse(void);

#endif
