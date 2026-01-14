#include "../../../../intf/stdint.h"

// GDT entry structure
typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed)) gdt_entry_t;

// GDT pointer structure
typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) gdt_ptr_t;

// GDT entries
static gdt_entry_t gdt[3];
static gdt_ptr_t gdt_ptr;

extern void gdt_load(uint64_t gdt_ptr_addr);

void gdt_init() {
    // Null descriptor
    gdt[0].limit_low = 0;
    gdt[0].base_low = 0;
    gdt[0].base_middle = 0;
    gdt[0].access = 0;
    gdt[0].granularity = 0;
    gdt[0].base_high = 0;

    // Code segment
    gdt[1].limit_low = 0xFFFF;
    gdt[1].base_low = 0;
    gdt[1].base_middle = 0;
    gdt[1].access = 0x9A; // Present, ring 0, code segment, executable, readable
    gdt[1].granularity = 0xAF; // 64-bit, 4KB granularity
    gdt[1].base_high = 0;

    // Data segment (must be 64-bit compatible)
    gdt[2].limit_low = 0xFFFF;
    gdt[2].base_low = 0;
    gdt[2].base_middle = 0;
    gdt[2].access = 0x92; // Present, ring 0, data segment, writable
    gdt[2].granularity = 0xAF; // 64-bit, 4KB granularity (was 0xCF - wrong for 64-bit!)
    gdt[2].base_high = 0;

    gdt_ptr.limit = sizeof(gdt) - 1;
    gdt_ptr.base = (uint64_t)&gdt;

    gdt_load((uint64_t)&gdt_ptr);

    // Reload segment registers with data segment selector
    __asm__ volatile (
        "mov $0x10, %ax\n"
        "mov %ax, %ds\n"
        "mov %ax, %es\n"
        "mov %ax, %fs\n"
        "mov %ax, %gs\n"
        "mov %ax, %ss\n"
    );
}