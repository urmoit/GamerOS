#include "../../intf/idt.h"
#include "../../intf/stdint.h"
#include "../../intf/ports.h"

#define IDT_ENTRIES 256

idt_entry_t idt[IDT_ENTRIES];
idt_ptr_t idt_ptr;

extern void idt_load(uint32_t);

// Interrupt service routines (ISRs) - defined in assembly
extern void isr0();
extern void isr1();
extern void isr2();
extern void isr3();
extern void isr4();
extern void isr5();
extern void isr6();
extern void isr7();
extern void isr8();
extern void isr9();
extern void isr10();
extern void isr11();
extern void isr12();
extern void isr13();
extern void isr14();
extern void isr15();
extern void isr16();
extern void isr17();
extern void isr18();
extern void isr19();
extern void isr20();
extern void isr21();
extern void isr22();
extern void isr23();
extern void isr24();
extern void isr25();
extern void isr26();
extern void isr27();
extern void isr28();
extern void isr29();
extern void isr30();
extern void isr31();

// IRQ handlers
extern void irq0();
extern void irq1();
extern void irq2();
extern void irq3();
extern void irq4();
extern void irq5();
extern void irq6();
extern void irq7();
extern void irq8();
extern void irq9();
extern void irq10();
extern void irq11();
extern void irq12();
extern void irq13();
extern void irq14();
extern void irq15();

void set_idt_entry(int n, uint32_t handler) {
    idt[n].isr_low = handler & 0xFFFF;
    idt[n].kernel_cs = 0x08; // Kernel code segment
    idt[n].reserved = 0;
    idt[n].attributes = 0x8E; // 32-bit interrupt gate
    idt[n].isr_high = (handler >> 16) & 0xFFFF;
}

void idt_init() {
    idt_ptr.limit = (sizeof(idt_entry_t) * IDT_ENTRIES) - 1;
    idt_ptr.base = (uint32_t)&idt;

    // Clear out the IDT
    for (int i = 0; i < IDT_ENTRIES; i++) {
        set_idt_entry(i, 0); // Set all to null handler initially
    }

    // Set up ISRs
    set_idt_entry(0, (uint32_t)isr0);
    set_idt_entry(1, (uint32_t)isr1);
    set_idt_entry(2, (uint32_t)isr2);
    set_idt_entry(3, (uint32_t)isr3);
    set_idt_entry(4, (uint32_t)isr4);
    set_idt_entry(5, (uint32_t)isr5);
    set_idt_entry(6, (uint32_t)isr6);
    set_idt_entry(7, (uint32_t)isr7);
    set_idt_entry(8, (uint32_t)isr8);
    set_idt_entry(9, (uint32_t)isr9);
    set_idt_entry(10, (uint32_t)isr10);
    set_idt_entry(11, (uint32_t)isr11);
    set_idt_entry(12, (uint32_t)isr12);
    set_idt_entry(13, (uint32_t)isr13);
    set_idt_entry(14, (uint32_t)isr14);
    set_idt_entry(15, (uint32_t)isr15);
    set_idt_entry(16, (uint32_t)isr16);
    set_idt_entry(17, (uint32_t)isr17);
    set_idt_entry(18, (uint32_t)isr18);
    set_idt_entry(19, (uint32_t)isr19);
    set_idt_entry(20, (uint32_t)isr20);
    set_idt_entry(21, (uint32_t)isr21);
    set_idt_entry(22, (uint32_t)isr22);
    set_idt_entry(23, (uint32_t)isr23);
    set_idt_entry(24, (uint32_t)isr24);
    set_idt_entry(25, (uint32_t)isr25);
    set_idt_entry(26, (uint32_t)isr26);
    set_idt_entry(27, (uint32_t)isr27);
    set_idt_entry(28, (uint32_t)isr28);
    set_idt_entry(29, (uint32_t)isr29);
    set_idt_entry(30, (uint32_t)isr30);
    set_idt_entry(31, (uint32_t)isr31);

    // Set up IRQs (hardware interrupts)
    // Remap PIC
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20); // Master PIC offset 0x20
    outb(0xA1, 0x28); // Slave PIC offset 0x28
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    outb(0x21, 0x0); // Mask all interrupts on master PIC
    outb(0xA1, 0x0); // Mask all interrupts on slave PIC

    set_idt_entry(32, (uint32_t)irq0);  // IRQ0: Timer
    set_idt_entry(33, (uint32_t)irq1);  // IRQ1: Keyboard
    set_idt_entry(34, (uint32_t)irq2);  // IRQ2: Cascade
    set_idt_entry(35, (uint32_t)irq3);  // IRQ3: COM2
    set_idt_entry(36, (uint32_t)irq4);  // IRQ4: COM1
    set_idt_entry(37, (uint32_t)irq5);  // IRQ5: LPT2
    set_idt_entry(38, (uint32_t)irq6);  // IRQ6: Floppy
    set_idt_entry(39, (uint32_t)irq7);  // IRQ7: LPT1
    set_idt_entry(40, (uint32_t)irq8);  // IRQ8: RTC
    set_idt_entry(41, (uint32_t)irq9);  // IRQ9: Free
    set_idt_entry(42, (uint32_t)irq10); // IRQ10: Free
    set_idt_entry(43, (uint32_t)irq11); // IRQ11: Free
    set_idt_entry(44, (uint32_t)irq12); // IRQ12: Mouse
    set_idt_entry(45, (uint32_t)irq13); // IRQ13: FPU
    set_idt_entry(46, (uint32_t)irq14); // IRQ14: Primary ATA
    set_idt_entry(47, (uint32_t)irq15); // IRQ15: Secondary ATA

    idt_load((uint32_t)&idt_ptr);
}