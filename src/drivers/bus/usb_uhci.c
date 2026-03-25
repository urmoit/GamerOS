#include <pci.h>
#include <ports.h>
#include <graphics.h>
#include <usb.h>

void uhci_init(uint8_t bus, uint8_t device, uint8_t func) {
    // 1. Get Base Address (BAR4 usually for UHCI)
    uint32_t bar4 = pci_read_config(bus, device, func, 0x20);
    uint16_t io_base = bar4 & 0xFFFE;

    // 2. Initial Reset
    outw(io_base + 0, 0x0002); // USBCMD: Global Reset
    for(int i=0; i<1000000; i++) __asm__("nop");
    outw(io_base + 0, 0x0000); // Stop Reset

    // 3. Scan Root Hub Ports (Usually 2 ports on UHCI)
    for (int port = 0; port < 2; port++) {
        uint16_t port_reg = io_base + 0x10 + (port * 2);
        uint16_t status = inw(port_reg);
        
        if (status & 0x0001) { // Current Connect Status
            vga_draw_string(10, 100 + (port * 10), "UHCI: Device detected on port!", XP_COLOR_WHITE);
            // In a real stack, we would now begin enumeration (Reset port, set address, etc)
        }
    }
}
