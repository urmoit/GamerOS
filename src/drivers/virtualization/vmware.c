#include <vmware.h>
#include <ports.h>
#include <graphics.h>
#include <mouse.h>
#include <serial.h>

// VMware absolute pointer (backdoor) driver.

typedef struct {
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
} vmware_regs_t;

static int vmware_active = 0;

static void vmware_backdoor(vmware_regs_t* regs) {
    __asm__ volatile (
        "in %%dx, %%eax"
        : "+a"(regs->eax), "+b"(regs->ebx), "+c"(regs->ecx), "+d"(regs->edx)
        :
        : "memory", "cc");
}

static void vmware_command(uint32_t command, uint32_t ebx_value, vmware_regs_t* regs) {
    regs->eax = VMWARE_MAGIC;
    regs->ebx = ebx_value;
    regs->ecx = command;
    regs->edx = VMWARE_PORT;
    vmware_backdoor(regs);
}

static int vmware_detect(void) {
    vmware_regs_t regs;

    // Non-VMware never answers: EBX holds the version, EAX echoes the magic.
    vmware_command(CMD_GET_VERSION, ~VMWARE_MAGIC, &regs);
    if (regs.ebx != VMWARE_MAGIC || (regs.eax & 0xFFFFFFFFu) != VMWARE_MAGIC) {
        return 0;
    }
    return 1;
}

static int vmware_enable_absolute(void) {
    vmware_regs_t regs;

    vmware_command(CMD_ABS_POINTER_COMMAND, ABS_POINTER_ENABLE, &regs);
    // EAX must echo the command, or the backdoor did not act.
    if (regs.eax != ABS_POINTER_ENABLE) {
        return 0;
    }

    vmware_command(CMD_ABS_POINTER_STATUS, 0, &regs);
    if (regs.eax & 0xFFFF0000u) {
        return 0;   // error flags in the status word
    }
    vmware_command(CMD_ABS_POINTER_DATA, 1, &regs);

    vmware_command(CMD_ABS_POINTER_COMMAND, ABS_POINTER_ABSOLUTE, &regs);
    if (regs.eax != ABS_POINTER_ABSOLUTE) {
        return 0;   // the device refused to switch to absolute reporting
    }
    return 1;
}

void vmware_mouse_init(void) {
    if (!vmware_detect()) {
        return;
    }

    if (!vmware_enable_absolute()) {
        serial_write_string("VMware: backdoor present, absolute pointer refused\n");
        return;
    }

    vmware_active = 1;
    serial_write_string("VMware: absolute pointer enabled\n");
}

int vmware_mouse_poll(void) {
    if (!vmware_active) return 0;

    vmware_regs_t regs;

    vmware_command(CMD_ABS_POINTER_STATUS, 0, &regs);
    uint32_t status = regs.eax;
    if ((status & 0xFFFF0000u) != 0) {
        return 0;   // error word set: the queue is unusable
    }
    if ((status & 0xFFFFu) < 4) {
        return 0;   // a full report needs four words
    }

    // Four words at once: status in EAX, X/Y/Z in EBX/ECX/EDX.
    vmware_command(CMD_ABS_POINTER_DATA, 4, &regs);

    uint16_t buttons = (uint16_t)(regs.eax & 0xFFFFu);
    // X and Y are 16-bit values carried in the low half of their register.
    uint32_t raw_x = regs.ebx & 0xFFFFu;
    uint32_t raw_y = regs.ecx & 0xFFFFu;
    int8_t wheel = (int8_t)(regs.edx & 0xFFu);

    mouse_set_position((int32_t)((raw_x * current_width) / 0xFFFFu),
                       (int32_t)((raw_y * current_height) / 0xFFFFu));

    if (wheel != 0) {
        mouse_inject_wheel(wheel);
    }

    uint8_t mapped = 0;
    if (buttons & ABS_STATUS_BUTTON_LEFT) mapped |= MOUSE_BTN_LEFT;
    if (buttons & ABS_STATUS_BUTTON_RIGHT) mapped |= MOUSE_BTN_RIGHT;
    if (buttons & ABS_STATUS_BUTTON_MIDDLE) mapped |= MOUSE_BTN_MIDDLE;
    mouse_set_buttons(mapped);

    return 1;
}

int vmware_mouse_active(void) {
    return vmware_active;
}
