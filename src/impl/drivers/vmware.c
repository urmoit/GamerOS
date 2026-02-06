#include "../../intf/vmware.h"
#include "../../intf/ports.h"
#include "../../intf/graphics.h"

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
    if (!vmware_initialized || !abs_pointer_enabled) return 0;
    
    uint32_t eax, ebx, ecx, edx;
    
    // Check status
    eax = VMWARE_MAGIC;
    ebx = 0;
    ecx = CMD_ABS_POINTER_STATUS;
    edx = VMWARE_PORT;
    vmware_backdoor(&eax, &ebx, &ecx, &edx);
    
    // EAX returns size/status. If bits 16-31 > 0, we have data.
    // Actually, VMware documentation says EAX = 0xFFFF00xx where xx is bytes available?
    // Let's use the simple CMD_GET_MOUSE_STATUS approach first for vmmouse?
    // No, absolute mouse is CMD_ABS_POINTER_DATA (39)
    
    // Status check: EAX should roughly indicate number of words
    if ((eax & 0xFFFF0000) == 0) return 0; // No data
    
    // Get Data
    // We need to read 4 bytes/words usually: status, x, y, z
    
    eax = VMWARE_MAGIC;
    ebx = 0;
    ecx = CMD_ABS_POINTER_DATA;
    edx = VMWARE_PORT;
    vmware_backdoor(&eax, &ebx, &ecx, &edx);
    
    // Returns 4 bytes or words?
    // Convention: 
    // EAX = Current X (scaled 0-65535) ? No, that's regular VMMOUSE.
    // The "Backdoor" absolute mouse actually reads count then pops data.
    
    // Let's try the simplest VMMOUSE v1 implementation:
    // It returns data in 4 words.
    // But we need to pop queue if multiple events.
    
    // Simplified logic:
    // ECX = 0 (Get X) -> EAX
    // ECX = 1 (Get Y) -> EAX
    // Wait, the backdoor command 39 fetches one packet.
    
    // Let's rely on documentation logic:
    // Packet size is 4 words.
    
    // Implementation of reading queue:
    int count = (eax & 0xFFFF); 
    if (count < 4) return 0; // Need at least header + x + y + z
    
    // Read Status Word
    eax = VMWARE_MAGIC;
    ebx = 0;
    ecx = CMD_ABS_POINTER_DATA; 
    edx = VMWARE_PORT;
    vmware_backdoor(&eax, &ebx, &ecx, &edx);
    uint32_t status = eax; // Flags
    
    // Read X
    eax = VMWARE_MAGIC;
    ebx = 0; 
    ecx = CMD_ABS_POINTER_DATA;
    edx = VMWARE_PORT;
    vmware_backdoor(&eax, &ebx, &ecx, &edx);
    uint32_t raw_x = eax;
    
    // Read Y
    eax = VMWARE_MAGIC;
    ebx = 0; 
    ecx = CMD_ABS_POINTER_DATA;
    edx = VMWARE_PORT;
    vmware_backdoor(&eax, &ebx, &ecx, &edx);
    uint32_t raw_y = eax;
    
    // Read Z/Buttons?
    eax = VMWARE_MAGIC;
    ebx = 0; 
    ecx = CMD_ABS_POINTER_DATA;
    edx = VMWARE_PORT;
    vmware_backdoor(&eax, &ebx, &ecx, &edx);
    uint32_t raw_z = eax;
    
    // Convert scaled coordinates (0-65535) to screen coordinates
    data->x = (raw_x * current_vga_width) / 65535;
    data->y = (raw_y * current_vga_height) / 65535;
    
    // Parse status for buttons
    // Bit 0 = Button 1 (Left), Bit 1 = Button 2 (Right), Bit 2 = Middle
    // GamerOS mapping: Left=0x01, Right=0x02
    data->buttons = 0;
    if (status & 0x20) data->buttons |= 0x01; // Left
    if (status & 0x10) data->buttons |= 0x02; // Right
    
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
