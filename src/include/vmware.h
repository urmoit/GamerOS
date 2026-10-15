#ifndef VMWARE_H
#define VMWARE_H

#include "stdint.h"

// VMware backdoor absolute pointer protocol.
//
// VMware answers a "low bandwidth" backdoor request read from I/O port 0x5658
// when EAX holds the magic. The command number is the low half of ECX, the
// payload goes in EBX, and all four registers come back with the answer. The
// absolute pointer commands report a 16-bit absolute position scaled to the
// display, so the driver maps it onto the desktop itself.
#define VMWARE_MAGIC 0x564D5868
#define VMWARE_PORT  0x5658

// Absolute pointer commands.
#define CMD_GET_VERSION              10
#define CMD_ABS_POINTER_COMMAND      41
#define CMD_ABS_POINTER_STATUS       40
#define CMD_ABS_POINTER_DATA         39

// Sub-commands of CMD_ABS_POINTER_COMMAND.
#define ABS_POINTER_ENABLE   0x45414552   /* enable the feature */
#define ABS_POINTER_RELATIVE 0xF5         /* back to relative PS/2 packets */
#define ABS_POINTER_ABSOLUTE 0x53424152   /* report absolute coordinates */

// Status word fields of an absolute pointer report.
#define ABS_STATUS_RELATIVE  0x00010000
#define ABS_STATUS_BUTTON_LEFT   0x20
#define ABS_STATUS_BUTTON_RIGHT  0x10
#define ABS_STATUS_BUTTON_MIDDLE 0x08

// Detect the backdoor and switch it to absolute reporting. Safe on a machine
// that is not running VMware: the port simply does not answer, and the read is
// harmless in ring 0.
void vmware_mouse_init(void);

// Read one queued absolute report and place the cursor. Returns 1 when a report
// was consumed. Buttons are not reported here: VMware keeps delivering those
// over the PS/2 port, which the existing mouse driver handles.
int vmware_mouse_poll(void);

int vmware_mouse_active(void);

#endif
