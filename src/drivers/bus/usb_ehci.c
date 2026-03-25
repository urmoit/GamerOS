#include <pci.h>
#include <ports.h>
#include <graphics.h>

void ehci_init(uint8_t bus, uint8_t device, uint8_t func) {
    uint32_t bar0 = pci_read_config(bus, device, func, 0x10);
    if (bar0 == 0) return;
    
    uint64_t mmio_base = bar0 & 0xFFFFFFF0;
    
    // 1. Take control from BIOS (Handover)
    // This is complex, but for now we'll just enable the controller
    
    // 2. Reset Controller
    // Standard EHCI registers are at offsets from mmio_base
    // We would need to map this memory in a real kernel with paging.
    // For now, assume identity mapping if small, or skip detailed MMIO.
}
