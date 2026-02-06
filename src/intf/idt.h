#ifndef IDT_H
#define IDT_H

#include "stdint.h"

// Structure for an IDT entry (64-bit)
typedef struct {
    uint16_t isr_low;       // Offset[15:0]
    uint16_t kernel_cs;     // Code segment selector
    uint8_t  ist;           // IST (bits 0-2), must be 0 if not using IST
    uint8_t  attributes;    // Type & attributes
    uint16_t isr_high;      // Offset[31:16]
    uint32_t isr_higher;    // Offset[63:32]
    uint32_t reserved;      // Reserved, must be 0
} __attribute__((packed)) idt_entry_t;

// Structure for the IDT register (64-bit)
typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) idt_ptr_t;

// Function to set an IDT entry
void set_idt_entry(int n, uint64_t handler);

// Function to initialize the IDT
void idt_init();

#endif