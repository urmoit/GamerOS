#include "../../../intf/hal.h"

void hal_init() {
    gdt_init();
    idt_init();
}

// TODO: Add more HAL initialization functions (PIC, PIT, etc.)
// TODO: Implement hardware abstraction for different architectures
// TODO: Add power management and ACPI support
// TODO: Implement device enumeration and configuration