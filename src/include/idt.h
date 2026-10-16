#ifndef IDT_H
#define IDT_H

#include "stdint.h"

typedef struct {
    uint16_t isr_low;
    uint16_t kernel_cs;
    uint8_t  ist;           // IST (bits 0-2), must be 0 if not using IST
    uint8_t  attributes;
    uint16_t isr_high;
    uint32_t isr_higher;
    uint32_t reserved;
} __attribute__((packed)) idt_entry_t;

typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) idt_ptr_t;

void set_idt_entry(int n, uint64_t handler);

void idt_init();

#endif