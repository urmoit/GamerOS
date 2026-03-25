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
