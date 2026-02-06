#include "../../../../intf/idt.h"
#include "../../../../intf/stdint.h"
#include "../../../../intf/ports.h"

#define IDT_ENTRIES 256
#define IDT_BASE_ADDRESS 0x0
#define PIC_MASTER_COMMAND 0x20
#define PIC_MASTER_DATA 0x21
#define PIC_SLAVE_COMMAND 0xA0
#define PIC_SLAVE_DATA 0xA1
#define PIC_MASTER_OFFSET 0x20
#define PIC_SLAVE_OFFSET 0x28
#define PIC_ICW1_ICW4 0x01
#define PIC_ICW1_INIT 0x10
#define PIC_ICW1_INTERVAL4 0x04
#define PIC_ICW1_SINGLE 0x02
#define PIC_ICW1_LEVEL 0x08
#define PIC_ICW4_8086 0x01
#define PIC_ICW4_AUTO 0x02
#define PIC_ICW4_BUF_SLAVE 0x08
#define PIC_ICW4_BUF_MASTER 0x0C
#define PIC_ICW4_SFNM 0x10
#define KERNEL_CODE_SEGMENT 0x08
#define INTERRUPT_GATE_64BIT 0x8E

idt_entry_t idt[IDT_ENTRIES];
idt_ptr_t idt_ptr;

extern void idt_load(uint64_t);

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

// System call handler
extern void syscall_stub();

void set_idt_entry(int n, uint64_t handler) {
    if (n < 0 || n >= IDT_ENTRIES) return; // Bounds check
    idt[n].isr_low = handler & 0xFFFF;
    idt[n].kernel_cs = KERNEL_CODE_SEGMENT;
    idt[n].ist = 0;                 // IST = 0 (don't use Interrupt Stack Table)
    idt[n].attributes = INTERRUPT_GATE_64BIT;
    idt[n].isr_high = (handler >> 16) & 0xFFFF;
    idt[n].isr_higher = (uint32_t)(handler >> 32);
    idt[n].reserved = 0;
}

void idt_init() {
    idt_ptr.limit = (sizeof(idt_entry_t) * IDT_ENTRIES) - 1;
    idt_ptr.base = (uintptr_t)&idt;  // Full 64-bit address

    // Clear out the IDT
    for (int i = 0; i < IDT_ENTRIES; i++) {
        set_idt_entry(i, 0); // Set all to null handler initially
    }

    // Set up ISRs
    set_idt_entry(0, (uintptr_t)isr0);
    set_idt_entry(1, (uintptr_t)isr1);
    set_idt_entry(2, (uintptr_t)isr2);
    set_idt_entry(3, (uintptr_t)isr3);
    set_idt_entry(4, (uintptr_t)isr4);
    set_idt_entry(5, (uintptr_t)isr5);
    set_idt_entry(6, (uintptr_t)isr6);
    set_idt_entry(7, (uintptr_t)isr7);
    set_idt_entry(8, (uintptr_t)isr8);
    set_idt_entry(9, (uintptr_t)isr9);
    set_idt_entry(10, (uintptr_t)isr10);
    set_idt_entry(11, (uintptr_t)isr11);
    set_idt_entry(12, (uintptr_t)isr12);
    set_idt_entry(13, (uintptr_t)isr13);
    set_idt_entry(14, (uintptr_t)isr14);
    set_idt_entry(15, (uintptr_t)isr15);
    set_idt_entry(16, (uintptr_t)isr16);
    set_idt_entry(17, (uintptr_t)isr17);
    set_idt_entry(18, (uintptr_t)isr18);
    set_idt_entry(19, (uintptr_t)isr19);
    set_idt_entry(20, (uintptr_t)isr20);
    set_idt_entry(21, (uintptr_t)isr21);
    set_idt_entry(22, (uintptr_t)isr22);
    set_idt_entry(23, (uintptr_t)isr23);
    set_idt_entry(24, (uintptr_t)isr24);
    set_idt_entry(25, (uintptr_t)isr25);
    set_idt_entry(26, (uintptr_t)isr26);
    set_idt_entry(27, (uintptr_t)isr27);
    set_idt_entry(28, (uintptr_t)isr28);
    set_idt_entry(29, (uintptr_t)isr29);
    set_idt_entry(30, (uintptr_t)isr30);
    set_idt_entry(31, (uintptr_t)isr31);

    // Set up IRQs (hardware interrupts)
    // Remap PIC
    outb(PIC_MASTER_COMMAND, PIC_ICW1_INIT | PIC_ICW1_ICW4);
    outb(PIC_SLAVE_COMMAND, PIC_ICW1_INIT | PIC_ICW1_ICW4);
    outb(PIC_MASTER_DATA, PIC_MASTER_OFFSET); // Master PIC offset
    outb(PIC_SLAVE_DATA, PIC_SLAVE_OFFSET); // Slave PIC offset
    outb(PIC_MASTER_DATA, 0x04); // Tell master about slave
    outb(PIC_SLAVE_DATA, 0x02); // Tell slave about master
    outb(PIC_MASTER_DATA, PIC_ICW4_8086);
    outb(PIC_SLAVE_DATA, PIC_ICW4_8086);
    outb(PIC_MASTER_DATA, 0xFF); // Mask all interrupts on master PIC initially
    outb(PIC_SLAVE_DATA, 0xFF); // Mask all interrupts on slave PIC initially

    // Don't enable interrupts yet - let the kernel enable them when ready
    // __asm__("sti"); // Commented out to prevent premature interrupt enabling

    set_idt_entry(32, (uintptr_t)irq0);  // IRQ0: Timer
    set_idt_entry(33, (uintptr_t)irq1);  // IRQ1: Keyboard
    set_idt_entry(34, (uintptr_t)irq2);  // IRQ2: Cascade
    set_idt_entry(35, (uintptr_t)irq3);  // IRQ3: COM2
    set_idt_entry(36, (uintptr_t)irq4);  // IRQ4: COM1
    set_idt_entry(37, (uintptr_t)irq5);  // IRQ5: LPT2
    set_idt_entry(38, (uintptr_t)irq6);  // IRQ6: Floppy
    set_idt_entry(39, (uintptr_t)irq7);  // IRQ7: LPT1
    set_idt_entry(40, (uintptr_t)irq8);  // IRQ8: RTC
    set_idt_entry(41, (uintptr_t)irq9);  // IRQ9: Free
    set_idt_entry(42, (uintptr_t)irq10); // IRQ10: Free
    set_idt_entry(43, (uintptr_t)irq11); // IRQ11: Free
    set_idt_entry(44, (uintptr_t)irq12); // IRQ12: Mouse
    set_idt_entry(45, (uintptr_t)irq13); // IRQ13: FPU
    set_idt_entry(46, (uintptr_t)irq14); // IRQ14: Primary ATA
    set_idt_entry(47, (uintptr_t)irq15); // IRQ15: Secondary ATA

    // Set up system call interrupt (int 0x80)
    set_idt_entry(0x80, (uintptr_t)syscall_stub);

    idt_load((uintptr_t)&idt_ptr);
}