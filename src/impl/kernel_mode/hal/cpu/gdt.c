#include "stdint.h"

// GDT already set up by boot.asm - we just need to ensure it's valid
// DO NOT reload GDT in 64-bit mode without properly reloading segment registers

void gdt_init() {
    // GDT is already set up by boot.asm
    // Reloading it in 64-bit mode requires a far jump to reload CS
    // which is complex and unnecessary since the boot GDT is correct
    
    // If we need to add user-mode segments or TSS later, we'll do it properly
    // with a full GDT reload including segment register updates
}
