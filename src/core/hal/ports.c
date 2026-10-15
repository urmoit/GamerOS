#include <ports.h>

// Hardware Abstraction Layer initialization
void hal_init(void) {
    // Minimal HAL initialization - just needs to exist for boot sequence
    // In a full system, this would initialize:
    // - Interrupt handlers
    // - Memory management
    // - Device drivers
}

// Read byte from I/O port
uint8_t inb(uint16_t port) {
    uint8_t val;
    __asm__ volatile("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

// Write byte to I/O port
void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

// Read word (16-bit) from I/O port
uint16_t inw(uint16_t port) {
    uint16_t val;
    __asm__ volatile("inw %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

// Write word (16-bit) to I/O port
void outw(uint16_t port, uint16_t val) {
    __asm__ volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

// Read double word (32-bit) from I/O port
uint32_t inl(uint16_t port) {
    uint32_t val;
    __asm__ volatile("inl %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

// Write double word (32-bit) to I/O port
void outl(uint16_t port, uint32_t val) {
    __asm__ volatile("outl %0, %1" : : "a"(val), "Nd"(port));
}

// I/O wait - small delay for I/O operations
void io_wait(void) {
    __asm__ volatile("jmp 1f\n1: jmp 2f\n2:");
}

// ----------------------------------------------------------------------------
// Memory-mapped register access
//
// The port-based helpers above address a 16-bit I/O port because that is all
// the legacy devices use. PCI devices such as USB host controllers expose their
// registers in memory instead, at a base address the PCI BAR supplies, which
// port instructions cannot reach at all: they take a 16-bit port number, so
// these are plain memory loads and stores against the bus address.
//
// The address is used whole. QEMU hands out every BAR in the low 4 GiB, which
// the kernel identity-maps, so the bus address is also a kernel address.
// ----------------------------------------------------------------------------

uint32_t mmio_inl(uint64_t address) {
    uint32_t val;
    __asm__ volatile(
        "movl (%1), %0"
        : "=r"(val)
        : "r"(address)
        : "memory");
    return val;
}

void mmio_outl(uint64_t address, uint32_t val) {
    __asm__ volatile(
        "movl %1, (%0)"
        :
        : "r"(address), "r"(val)
        : "memory");
}

uint32_t mmio_inw(uint64_t address) {
    uint32_t val;
    __asm__ volatile(
        "movzwl (%1), %0"
        : "=r"(val)
        : "r"(address)
        : "memory");
    return val;
}

void mmio_outw(uint64_t address, uint32_t val) {
    __asm__ volatile(
        "movw %w1, (%0)"
        :
        : "r"(address), "r"(val)
        : "memory");
}