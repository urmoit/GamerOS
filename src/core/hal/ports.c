#include <ports.h>

void hal_init(void) {
}

uint8_t inb(uint16_t port) {
    uint8_t val;
    __asm__ volatile("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

uint16_t inw(uint16_t port) {
    uint16_t val;
    __asm__ volatile("inw %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

void outw(uint16_t port, uint16_t val) {
    __asm__ volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

uint32_t inl(uint16_t port) {
    uint32_t val;
    __asm__ volatile("inl %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

void outl(uint16_t port, uint32_t val) {
    __asm__ volatile("outl %0, %1" : : "a"(val), "Nd"(port));
}

void io_wait(void) {
    __asm__ volatile("jmp 1f\n1: jmp 2f\n2:");
}

/* ---- MMIO register access ---- */

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