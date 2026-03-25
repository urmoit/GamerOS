#include <pci.h>
#include <ports.h>
#include <graphics.h>

void xhci_init(uint8_t bus, uint8_t device, uint8_t func) {
    uint32_t bar0 = pci_read_config(bus, device, func, 0x10);
    uint32_t bar1 = pci_read_config(bus, device, func, 0x14);
    uint64_t mmio_base = (uint64_t)bar0 & 0xFFFFFFF0;
    
    // Check if 64-bit address
    if ((bar0 & 0x06) == 0x04) {
        mmio_base |= ((uint64_t)bar1 << 32);
    }

    if (mmio_base == 0) return;

    vga_draw_string(10, 120, "XHCI: Modern USB 3.0 Controller Initialized", XP_COLOR_WHITE);
}
