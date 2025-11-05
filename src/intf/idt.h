#ifndef IDT_H
#define IDT_H

#include "stdint.h"

// Structure for an IDT entry
typedef struct {
    uint16_t isr_low;
    uint16_t kernel_cs;
    uint8_t  reserved;
    uint8_t  attributes;
    uint16_t isr_high;
} __attribute__((packed)) idt_entry_t;

// Structure for the IDT register
typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idt_ptr_t;

// Function to set an IDT entry
void set_idt_entry(int n, uint32_t handler);

// Function to initialize the IDT
void idt_init();

#endif