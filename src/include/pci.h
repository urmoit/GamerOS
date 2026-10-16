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

// Mechanism 1 config space access, 32-bit aligned reads.
uint32_t pci_read_config(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
void pci_write_config(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value);

uint32_t pci_read_reg(pci_device_t* dev, uint8_t offset);
void pci_write_reg(pci_device_t* dev, uint8_t offset, uint32_t value);

void pci_enable_bus_mastering(pci_device_t* dev);

// A BAR with bit 0 set is an I/O region; memory BARs are masked with 0xFFFFFFF0.
uint32_t pci_get_bar(pci_device_t* dev, int bar_index);

// prog_if = -1 matches any programming interface; returns 1 on success.
int pci_find_class(uint8_t class_code, uint8_t subclass, int prog_if, pci_device_t* out);

// Callback returns non-zero to stop enumeration early.
void pci_scan_class(uint8_t class_code, uint8_t subclass, int prog_if,
                    int (*callback)(pci_device_t* dev, void* ctx), void* ctx);

int pci_find_device(uint16_t vendor_id, uint16_t device_id, pci_device_t* out);

void pci_init(void);

#endif
