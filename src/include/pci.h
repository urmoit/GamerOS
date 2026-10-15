#ifndef PCI_H
#define PCI_H

#include "stdint.h"

typedef struct {
    uint8_t bus;
    uint8_t device;
    uint8_t function;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t class_code;
    uint8_t subclass;
    uint8_t prog_if;
} pci_device_t;

// PCI configuration space access (mechanism 1, 32-bit aligned reads).
uint32_t pci_read_config(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
void pci_write_config(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value);

// Configuration space helpers. Addresses are 32-bit register values with the
// low bits already masked off by pci_read_config, so callers keep the low two
// bits of the offset when combining BAR halves.
uint32_t pci_read_reg(pci_device_t* dev, uint8_t offset);
void pci_write_reg(pci_device_t* dev, uint8_t offset, uint32_t value);

// Turn on bus mastering plus I/O and memory decoding for a device. DMA-capable
// controllers (USB host controllers, the VMM device) must be masters before
// they will service their queues.
void pci_enable_bus_mastering(pci_device_t* dev);

// Address and size of one Base Address Register. A BAR that returns an I/O
// region has bit 0 set and is reported masked with 0xFFFFFFFC; a memory BAR is
// masked with 0xFFFFFFF0 and can span two registers (64-bit).
uint32_t pci_get_bar(pci_device_t* dev, int bar_index);

// Find the first device matching a class/subclass, optionally pinned to a
// programming interface. Pass prog_if = -1 for "any". Returns 1 on success.
int pci_find_class(uint8_t class_code, uint8_t subclass, int prog_if, pci_device_t* out);

// Visit every device of a class in bus order, calling the callback for each.
// The callback returns non-zero to stop enumeration early, which is how a
// driver that only handles N controllers stops the walk. Undefined behaviour
// if pci_find_class is called from the callback.
void pci_scan_class(uint8_t class_code, uint8_t subclass, int prog_if,
                    int (*callback)(pci_device_t* dev, void* ctx), void* ctx);

// Find the first device matching a vendor and device ID. Returns 1 on success.
int pci_find_device(uint16_t vendor_id, uint16_t device_id, pci_device_t* out);

void pci_init(void);

#endif
