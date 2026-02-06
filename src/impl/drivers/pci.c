#include "../../intf/pci.h"
#include "../../intf/ports.h"
#include "../../intf/graphics.h"

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

extern void uhci_init(uint8_t bus, uint8_t device, uint8_t func);
extern void ehci_init(uint8_t bus, uint8_t device, uint8_t func);
extern void xhci_init(uint8_t bus, uint8_t device, uint8_t func);

uint32_t pci_read_config(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    uint32_t address = (uint32_t)((uint32_t)bus << 16) | 
                       (uint32_t)((uint32_t)device << 11) | 
                       (uint32_t)((uint32_t)function << 8) | 
                       (uint32_t)(offset & 0xFC) | 
                       ((uint32_t)0x80000000);
    outl(PCI_CONFIG_ADDRESS, address);
    return inl(PCI_CONFIG_DATA);
}

void pci_write_config(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value) {
    uint32_t address = (uint32_t)((uint32_t)bus << 16) | 
                       (uint32_t)((uint32_t)device << 11) | 
                       (uint32_t)((uint32_t)function << 8) | 
                       (uint32_t)(offset & 0xFC) | 
                       ((uint32_t)0x80000000);
    outl(PCI_CONFIG_ADDRESS, address);
    outl(PCI_CONFIG_DATA, value);
}

void pci_init(void) {
    // Basic scan and print detected USB controllers to debug console or screen
    // For now, we'll just scan and identify USB controllers
    for (uint32_t bus = 0; bus < 256; bus++) {
        for (uint32_t device = 0; device < 32; device++) {
            uint32_t reg0 = pci_read_config(bus, device, 0, 0);
            uint16_t vendor_id = reg0 & 0xFFFF;
            if (vendor_id == 0xFFFF) continue;

            uint32_t reg8 = pci_read_config(bus, device, 0, 8);
            uint8_t class_code = (reg8 >> 24) & 0xFF;
            uint8_t subclass = (reg8 >> 16) & 0xFF;
            uint8_t prog_if = (reg8 >> 8) & 0xFF;

            if (class_code == 0x0C && subclass == 0x03) {
                // USB Controller detected
                const char* type = "Unknown";
                if (prog_if == 0x00) type = "UHCI (USB 1.1)";
                else if (prog_if == 0x10) type = "OHCI (USB 1.1)";
                else if (prog_if == 0x20) type = "EHCI (USB 2.0)";
                else if (prog_if == 0x30) type = "XHCI (USB 3.0)";

                // Enable Bus Mastering and Memory/IO Space
                uint32_t command = pci_read_config(bus, device, 0, 0x04);
                command |= 0x07; // Bus Master, Memory Space, I/O Space
                pci_write_config(bus, device, 0, 0x04, command);

                char log[64];
                // Simple string building since we don't have sprintf
                vga_draw_string(10, 10 + (bus * 2) + (device), "Found: ", XP_COLOR_WHITE);
                vga_draw_string(60, 10 + (bus * 2) + (device), type, XP_COLOR_WHITE);

                // Initialize specific driver
                if (prog_if == 0x00) uhci_init(bus, device, 0);
                else if (prog_if == 0x20) ehci_init(bus, device, 0);
                else if (prog_if == 0x30) xhci_init(bus, device, 0);
                
                // Small delay to let the user see the log
                for(int i=0; i<1000000; i++) __asm__("nop");
            }
        }
    }
}
