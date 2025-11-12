// isr.c - Interrupt Service Routine handlers
#include "../../intf/stdint.h"
#include "../../intf/ports.h"
#include "../../intf/pic.h"

#define VGA_TEXT_BUFFER 0xB8000
#define EXCEPTION_COUNT 32
#define TIMER_IRQ 32
#define KEYBOARD_IRQ 33
#define MOUSE_IRQ 44

// Global system tick counter for proper timer interrupts
volatile uint64_t system_ticks = 0;

// System call interface
#define SYSCALL_READ    0
#define SYSCALL_WRITE   1
#define SYSCALL_OPEN    2
#define SYSCALL_CLOSE   3
#define SYSCALL_EXIT    4

// System call handler (declared as extern in isr.asm)
void syscall_handler(uint64_t syscall_num, uint64_t arg1, uint64_t arg2, uint64_t arg3) {
    switch (syscall_num) {
        case SYSCALL_WRITE:
            // Simple write syscall - for now just handle to stdout (screen)
            // In a real system, this would write to files, etc.
            if (arg1 == 1) { // stdout
                const char* str = (const char*)arg2;
                // Simple screen output (would be more sophisticated in real OS)
                char* video_memory = (char*)0xB8000;
                for (size_t i = 0; str[i] && i < (size_t)arg3; i++) {
                    video_memory[i * 2] = str[i];
                    video_memory[i * 2 + 1] = 0x07; // White on black
                }
            }
            break;
        case SYSCALL_EXIT:
            // Process exit syscall
            extern void terminate_process(int);
            terminate_process((int)arg1); // arg1 = process ID
            break;
        // Other syscalls would be implemented here
        default:
            // Unknown syscall
            break;
    }
}

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

// Common ISR handler (declared as extern in isr.asm)
void common_isr_handler(registers_t regs) {
    // Print exception message to screen
    char* video_memory = (char*)VGA_TEXT_BUFFER;
    const char* msg = "Exception: ";
    size_t i = 0;
    for (; msg[i] != '\0'; i++) {
        video_memory[i * 2] = msg[i];
        video_memory[i * 2 + 1] = 0x4F; // White on red
    }

    if (regs.int_no < EXCEPTION_COUNT) {
        const char* exc_msg = exception_messages[regs.int_no];
        if (exc_msg) { // NULL check
            size_t j = 0;
            for (; exc_msg[j] != '\0'; j++) {
                video_memory[(i + j) * 2] = exc_msg[j];
                video_memory[(i + j) * 2 + 1] = 0x4F;
            }
        }
    }

    // Handle page faults specifically - try to continue instead of halting
    if (regs.int_no == 14) { // Page fault
        // For now, just print and continue - in a real OS we'd handle this properly
        const char* pf_msg = " Page Fault Handled";
        size_t k = 0;
        for (; pf_msg[k] != '\0'; k++) {
            video_memory[(i + 20 + k) * 2] = pf_msg[k];
            video_memory[(i + 20 + k) * 2 + 1] = 0x4E; // Yellow on red
        }
        return; // Continue execution instead of halting
    }

    // For other exceptions, halt the system
    for (;;) {
        __asm__("hlt");
    }
}

// Common IRQ handler (declared as extern in isr.asm)
void common_irq_handler(registers_t regs) {
    // Send EOI to PIC using proper function
    pic_eoi(regs.int_no - 32); // Convert interrupt number to IRQ number

    // Handle specific IRQs
    switch (regs.int_no) {
        case TIMER_IRQ: // Timer interrupt - proper timer handling
            // Increment system tick counter
            extern volatile uint64_t system_ticks;
            system_ticks++;

            // Trigger scheduler for preemptive multitasking
            extern void schedule();
            schedule();
            break;
        case KEYBOARD_IRQ: // Keyboard interrupt
            extern void keyboard_handler();
            keyboard_handler();
            break;
        case MOUSE_IRQ: // Mouse interrupt
            extern void mouse_handler();
            mouse_handler();
            break;
        // Other IRQs can be handled here as needed
        default:
            // No action needed for unhandled IRQs
            break;
    }
}