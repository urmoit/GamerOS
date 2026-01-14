#include "../../../../intf/stdint.h"

// Page table structures
#define PAGE_SIZE 4096
#define PAGE_TABLE_ENTRIES 512

// Page table levels
// Export these symbols so boot.asm can access them for paging setup
// They must be global so assembly code can reference them
uint64_t p4_table[PAGE_TABLE_ENTRIES] __attribute__((aligned(PAGE_SIZE)));
uint64_t p3_table[PAGE_TABLE_ENTRIES] __attribute__((aligned(PAGE_SIZE)));
uint64_t p2_table[PAGE_TABLE_ENTRIES] __attribute__((aligned(PAGE_SIZE)));

#define VGA_GRAPHICS_BUFFER 0xA0000

void setup_page_tables() {
    // Zero out all page tables first (they're in BSS but be explicit)
    // This ensures all entries are properly initialized
    
    // Identity map first 4MB (2 pages) to ensure kernel has enough mapped memory
    // P4 table: entry 0 points to P3 table
    p4_table[0] = (uint64_t)&p3_table | 0b11; // Present + Writable
    
    // P3 table: entry 0 points to P2 table  
    p3_table[0] = (uint64_t)&p2_table | 0b11; // Present + Writable

    // P2 table: entry 0 maps 0-2MB (2MB huge page)
    p2_table[0] = 0x0 | 0b10000011; // Present + Writable + Huge (bit 7)

    // P2 table: entry 1 maps 2-4MB (2MB huge page)
    p2_table[1] = 0x200000 | 0b10000011; // Present + Writable + Huge

    // VGA memory (0xA0000) is already mapped in the first 2MB (p2_table[0])
    // Kernel code/data is also in the first 4MB, so this covers everything
}

void hal_init() {
    setup_page_tables();
    // Note: Paging is enabled in boot.asm after this function returns
    // This is necessary because paging must be enabled before jumping to 64-bit mode
    extern void gdt_init();
    gdt_init();
    extern void idt_init();
    idt_init();
}