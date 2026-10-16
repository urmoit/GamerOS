#include <pci.h>
#include <ports.h>

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

#define PCI_VENDOR_ID      0x00
#define PCI_DEVICE_ID      0x02
#define PCI_COMMAND        0x04
#define PCI_REVISION_ID    0x08
#define PCI_PROG_IF        0x09
#define PCI_SUBCLASS       0x0A
#define PCI_CLASS_CODE     0x0B
#define PCI_HEADER_TYPE    0x0E
#define PCI_BAR0           0x10

#define PCI_CMD_BUS_MASTER 0x0004
#define PCI_CMD_IO_SPACE   0x0001
#define PCI_CMD_MEM_SPACE  0x0002

uint32_t pci_read_config(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    uint32_t address = ((uint32_t)bus << 16) |
                       ((uint32_t)device << 11) |
                       ((uint32_t)function << 8) |
                       (uint32_t)(offset & 0xFC) |
                       0x80000000u;
    outl(PCI_CONFIG_ADDRESS, address);
    return inl(PCI_CONFIG_DATA);
}

void pci_write_config(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value) {
    uint32_t address = ((uint32_t)bus << 16) |
                       ((uint32_t)device << 11) |
                       ((uint32_t)function << 8) |
                       (uint32_t)(offset & 0xFC) |
                       0x80000000u;
    outl(PCI_CONFIG_ADDRESS, address);
    outl(PCI_CONFIG_DATA, value);
}

uint32_t pci_read_reg(pci_device_t* dev, uint8_t offset) {
    return pci_read_config(dev->bus, dev->device, dev->function, offset);
}

void pci_write_reg(pci_device_t* dev, uint8_t offset, uint32_t value) {
    pci_write_config(dev->bus, dev->device, dev->function, offset, value);
}

void pci_enable_bus_mastering(pci_device_t* dev) {
    uint32_t command = pci_read_reg(dev, PCI_COMMAND);
    command |= PCI_CMD_BUS_MASTER | PCI_CMD_IO_SPACE | PCI_CMD_MEM_SPACE;
    pci_write_reg(dev, PCI_COMMAND, command);
}

uint32_t pci_get_bar(pci_device_t* dev, int bar_index) {
    uint8_t offset = (uint8_t)(PCI_BAR0 + bar_index * 4);
    return pci_read_reg(dev, offset);
}

// Vendor 0xFFFF means the function is absent.
static int pci_probe(pci_device_t* out, uint8_t bus, uint8_t device, uint8_t function) {
    uint32_t reg0 = pci_read_config(bus, device, function, 0);
    if ((reg0 & 0xFFFF) == 0xFFFF) {
        return 0;
    }

    uint32_t reg_rev = pci_read_config(bus, device, function, PCI_REVISION_ID);
    uint32_t reg_id = pci_read_config(bus, device, function, PCI_DEVICE_ID);

    out->bus = bus;
    out->device = device;
    out->function = function;
    out->vendor_id = (uint16_t)(reg0 & 0xFFFF);
    out->device_id = (uint16_t)(reg_id >> 16);
    out->class_code = (uint8_t)((reg_rev >> 24) & 0xFF);
    out->subclass = (uint8_t)((reg_rev >> 16) & 0xFF);
    out->prog_if = (uint8_t)((reg_rev >> 8) & 0xFF);
    return 1;
}

int pci_find_class(uint8_t class_code, uint8_t subclass, int prog_if, pci_device_t* out) {
    for (uint32_t bus = 0; bus < 256; bus++) {
        for (uint32_t device = 0; device < 32; device++) {
            pci_device_t found;
            if (!pci_probe(&found, (uint8_t)bus, (uint8_t)device, 0)) {
                continue;
            }
            if (found.class_code == class_code && found.subclass == subclass &&
                (prog_if < 0 || found.prog_if == (uint8_t)prog_if)) {
                if (out) *out = found;
                return 1;
            }
            // Header type bit 7 marks a multi-function device.
            uint8_t header_type = (uint8_t)(pci_read_config((uint8_t)bus, (uint8_t)device, 0, PCI_HEADER_TYPE) >> 16);
            if (!(header_type & 0x80)) {
                continue;
            }
            for (uint8_t function = 1; function < 8; function++) {
                if (!pci_probe(&found, (uint8_t)bus, (uint8_t)device, function)) {
                    continue;
                }
                if (found.class_code == class_code && found.subclass == subclass &&
                    (prog_if < 0 || found.prog_if == (uint8_t)prog_if)) {
                    if (out) *out = found;
                    return 1;
                }
            }
        }
    }
    return 0;
}

int pci_find_device(uint16_t vendor_id, uint16_t device_id, pci_device_t* out) {
    for (uint32_t bus = 0; bus < 256; bus++) {
        for (uint32_t device = 0; device < 32; device++) {
            pci_device_t found;
            if (!pci_probe(&found, (uint8_t)bus, (uint8_t)device, 0)) {
                continue;
            }
            if (found.vendor_id == vendor_id && found.device_id == device_id) {
                if (out) *out = found;
                return 1;
            }
            uint8_t header_type = (uint8_t)(pci_read_config((uint8_t)bus, (uint8_t)device, 0, PCI_HEADER_TYPE) >> 16);
            if (!(header_type & 0x80)) {
                continue;
            }
            for (uint8_t function = 1; function < 8; function++) {
                if (!pci_probe(&found, (uint8_t)bus, (uint8_t)device, function)) {
                    continue;
                }
                if (found.vendor_id == vendor_id && found.device_id == device_id) {
                    if (out) *out = found;
                    return 1;
                }
            }
        }
    }
    return 0;
}

void pci_scan_class(uint8_t class_code, uint8_t subclass, int prog_if,
                    int (*callback)(pci_device_t* dev, void* ctx), void* ctx) {
    for (uint32_t bus = 0; bus < 256; bus++) {
        for (uint32_t device = 0; device < 32; device++) {
            pci_device_t found;
            if (!pci_probe(&found, (uint8_t)bus, (uint8_t)device, 0)) {
                continue;
            }
            if (found.class_code == class_code && found.subclass == subclass &&
                (prog_if < 0 || found.prog_if == (uint8_t)prog_if)) {
                if (callback(&found, ctx)) return;
            }
            uint8_t header_type = (uint8_t)(pci_read_config((uint8_t)bus, (uint8_t)device, 0, PCI_HEADER_TYPE) >> 16);
            if (!(header_type & 0x80)) {
                continue;
            }
            for (uint8_t function = 1; function < 8; function++) {
                if (!pci_probe(&found, (uint8_t)bus, (uint8_t)device, function)) {
                    continue;
                }
                if (found.class_code == class_code && found.subclass == subclass &&
                    (prog_if < 0 || found.prog_if == (uint8_t)prog_if)) {
                    if (callback(&found, ctx)) return;
                }
            }
        }
    }
}
