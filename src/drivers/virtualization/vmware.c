#include <vmware.h>
#include <ports.h>
#include <graphics.h>

// Global state
static int vmware_initialized = 0;
static int abs_pointer_enabled = 0;

// Low-level Backdoor Communication
// We need inline assembly because C functions can't easily access specific registers
// for IN/OUT instructions with exact register constraints required by VMware.

static void vmware_backdoor(uint32_t* eax, uint32_t* ebx, uint32_t* ecx, uint32_t* edx) {
    // VMware Backdoor Calling Convention:
    // EAX = Command (High 16 bits = Magic, Low 16 bits = Command)
    // EBX = Parameter 1
    // ECX = Parameter 2
    // DX  = Port (0x5658)
    //
    // Returns data in EAX, EBX, ECX, EDX
    
    __asm__ volatile (
        "in %%dx, %%eax"
        : "+a" (*eax), "+b" (*ebx), "+c" (*ecx), "+d" (*edx)
        : 
        : "memory"
    );
}

int vmware_detect(void) {
    uint32_t eax, ebx, ecx, edx;
    
    eax = VMWARE_MAGIC;
    ebx = 0; // Parameter
    ecx = CMD_GET_VERSION;
    edx = VMWARE_PORT;
    
    // We need to catch potential GPF if not in VMware (e.g. real hardware)
    // For now, we assume if detecting, we might be safe or this runs early.
    // In a robust OS, we'd set up a temporary exception handler.
    // For GamerOS simple loop, we'll rely on CPUID first or try raw.
    // NOTE: Sending to port 0x5658 on non-VMware usually does nothing, 
    // but without GPF handler it might crash if IO port is privileged? 
    // In Ring 0 it's usually fine.
    
    vmware_backdoor(&eax, &ebx, &ecx, &edx);
    
    if (ebx == VMWARE_MAGIC) {
        vmware_initialized = 1;
        return 1;
    }
    
    return 0;
}

void vmware_enable_mouse(void) {
    if (!vmware_initialized) return;
    
    uint32_t eax, ebx, ecx, edx;
    
    // Enable absolute pointer
    eax = VMWARE_MAGIC;
    ebx = ABS_POINTER_ENABLE;
    ecx = CMD_ABS_POINTER_COMMAND;
    edx = VMWARE_PORT;
    vmware_backdoor(&eax, &ebx, &ecx, &edx);
    
    // Request absolute data
    eax = VMWARE_MAGIC;
    ebx = ABS_POINTER_ABSOLUTE;
    ecx = CMD_ABS_POINTER_COMMAND;
    edx = VMWARE_PORT;
    vmware_backdoor(&eax, &ebx, &ecx, &edx);
    
    abs_pointer_enabled = 1;
}

int vmware_get_mouse_data(vmware_mouse_data_t* data) {
    if (!vmware_initialized || !abs_pointer_enabled || !data) return 0;

    uint32_t eax, ebx, ecx, edx;

    // Read the queue status. EAX[31:16] holds the number of words available.
    eax = VMWARE_MAGIC;
    ebx = 0;
    ecx = CMD_ABS_POINTER_STATUS;
    edx = VMWARE_PORT;
    vmware_backdoor(&eax, &ebx, &ecx, &edx);

    uint32_t words_available = (eax >> 16) & 0xFFFF;
    if (words_available < 4) return 0; // need status + x + y + z

    // Each CMD_ABS_POINTER_DATA call pops one word from the queue.
    uint32_t packet[4];
    for (int i = 0; i < 4; i++) {
        eax = VMWARE_MAGIC;
        ebx = 0;
        ecx = CMD_ABS_POINTER_DATA;
        edx = VMWARE_PORT;
        vmware_backdoor(&eax, &ebx, &ecx, &edx);
        packet[i] = eax;
    }

    uint32_t status = packet[0];
    uint32_t raw_x = packet[1];
    uint32_t raw_y = packet[2];

    // Coordinates arrive as unsigned 0..65535. Cast before scaling so the
    // multiply is done in 64-bit and cannot wrap.
    uint64_t screen_w = current_vga_width ? current_vga_width : 1;
    uint64_t screen_h = current_vga_height ? current_vga_height : 1;

    int64_t scaled_x = (int64_t)(((uint64_t)raw_x * screen_w) / 65535ULL);
    int64_t scaled_y = (int64_t)(((uint64_t)raw_y * screen_h) / 65535ULL);

    if (scaled_x < 0) scaled_x = 0;
    if (scaled_y < 0) scaled_y = 0;
    if (scaled_x > (int64_t)screen_w) scaled_x = (int64_t)screen_w;
    if (scaled_y > (int64_t)screen_h) scaled_y = (int64_t)screen_h;

    data->x = (int32_t)scaled_x;
    data->y = (int32_t)scaled_y;
    data->scroll = (int32_t)packet[3];

    // Status bit layout: 0x20 = left, 0x10 = right, 0x08 = middle.
    data->buttons = 0;
    if (status & 0x20) data->buttons |= 0x01;
    if (status & 0x10) data->buttons |= 0x02;
    if (status & 0x08) data->buttons |= 0x04;

    return 1;
}

void vmware_disable_mouse(void) {
    if (!vmware_initialized) return;
    
    uint32_t eax, ebx, ecx, edx;
    eax = VMWARE_MAGIC;
    ebx = 0;
    ecx = CMD_ABS_POINTER_COMMAND; // Disable
    // Parameter for disable is 0x000000f5 (Relative)
    ebx = ABS_POINTER_RELATIVE;
    edx = VMWARE_PORT;
    vmware_backdoor(&eax, &ebx, &ecx, &edx);
    
    abs_pointer_enabled = 0;
}
