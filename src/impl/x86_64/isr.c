// isr.c - Interrupt Service Routine handlers
#include "../../intf/stdint.h"

// Forward declarations for ISR handlers
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

// Exception messages
static const char* exception_messages[] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Into Detected Overflow",
    "Out of Bounds",
    "Invalid Opcode",
    "No Coprocessor",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Bad TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",
    "Coprocessor Fault",
    "Alignment Check",
    "Machine Check",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved"
};

// Registers structure for interrupt context
typedef struct {
    uint64_t ds;
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rsi, rdi, rdx, rcx, rbx, rax;
    uint64_t int_no, err_code;
    uint64_t rip, cs, rflags, rsp, ss;
} registers_t;

// Common ISR handler
void common_isr_handler(registers_t regs) {
    // Print exception message to screen
    char* video_memory = (char*)0xB8000;
    const char* msg = "Exception: ";
    int i = 0;
    for (; msg[i] != '\0'; i++) {
        video_memory[i * 2] = msg[i];
        video_memory[i * 2 + 1] = 0x4F; // White on red
    }

    if (regs.int_no < 32) {
        const char* exc_msg = exception_messages[regs.int_no];
        for (int j = 0; exc_msg[j] != '\0'; j++) {
            video_memory[(i + j) * 2] = exc_msg[j];
            video_memory[(i + j) * 2 + 1] = 0x4F;
        }
    }

    // Halt the system
    for (;;) {
        __asm__("hlt");
    }
}

// Common IRQ handler
void common_irq_handler(registers_t regs) {
    // Send EOI to PIC
    if (regs.int_no >= 40) {
        // Send reset signal to slave PIC
        outb(0xA0, 0x20);
    }
    // Send reset signal to master PIC
    outb(0x20, 0x20);

    // Handle specific IRQs here if needed
    // For now, just return
}