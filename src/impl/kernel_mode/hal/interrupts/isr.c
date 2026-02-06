// isr.c - Interrupt Service Routine handlers
#include "../../../../intf/stdint.h"
#include "../../../../intf/ports.h"
#include "../../../../intf/pic.h"

#define VGA_TEXT_BUFFER 0xB8000
#define EXCEPTION_COUNT 32
#define TIMER_IRQ 32
#define KEYBOARD_IRQ 33
#define MOUSE_IRQ 44

// System call interface
#define SYSCALL_READ    0
#define SYSCALL_WRITE   1
#define SYSCALL_OPEN    2
#define SYSCALL_CLOSE   3
#define SYSCALL_EXIT    4

// Stub functions
void terminate_process(int code) { (void)code; }
void schedule(void) { }

// System call handler
void syscall_handler(uint64_t syscall_num, uint64_t arg1, uint64_t arg2, uint64_t arg3) {
    (void)arg1; (void)arg2; (void)arg3;
    switch (syscall_num) {
        case SYSCALL_WRITE:
            break;
        case SYSCALL_EXIT:
            terminate_process((int)arg1);
            break;
        default:
            break;
    }
}

// Exception messages
static const char* exception_messages[] = {
    "Division By Zero", "Debug", "Non Maskable Interrupt", "Breakpoint",
    "Into Detected Overflow", "Out of Bounds", "Invalid Opcode", "No Coprocessor",
    "Double Fault", "Coprocessor Segment Overrun", "Bad TSS", "Segment Not Present",
    "Stack Fault", "General Protection Fault", "Page Fault", "Unknown Interrupt",
    "Coprocessor Fault", "Alignment Check", "Machine Check", "Reserved",
    "Reserved", "Reserved", "Reserved", "Reserved",
    "Reserved", "Reserved", "Reserved", "Reserved",
    "Reserved", "Reserved", "Reserved", "Reserved"
};

// Common ISR handler - called from assembly
// RDI contains pointer to register structure on stack
void common_isr_handler(uint64_t* regs) {
    uint64_t int_no = regs[15]; // interrupt number is at offset 15 (after 15 saved regs)
    
    // Silently ignore spurious NMIs
    if (int_no == 2) return;
    
    // Print exception message
    char* video_memory = (char*)VGA_TEXT_BUFFER;
    const char* msg = "Exception: ";
    for (size_t i = 0; msg[i]; i++) {
        video_memory[i * 2] = msg[i];
        video_memory[i * 2 + 1] = 0x4F;
    }
    
    if (int_no < EXCEPTION_COUNT && exception_messages[int_no]) {
        const char* exc_msg = exception_messages[int_no];
        for (size_t j = 0; exc_msg[j]; j++) {
            video_memory[(11 + j) * 2] = exc_msg[j];
            video_memory[(11 + j) * 2 + 1] = 0x4F;
        }
    }
    
    // Handle page faults and GPF
    if (int_no == 14 || int_no == 13) {
        return; // Try to continue
    }
    
    // Halt on other exceptions
    for (;;) { __asm__("hlt"); }
}

// Common IRQ handler - called from assembly
void common_irq_handler(uint64_t* regs) {
    uint64_t int_no = regs[15]; // interrupt number
    uint8_t irq = (uint8_t)(int_no - 32);
    
    // Send EOI first
    pic_eoi(irq);
    
    // Handle specific IRQs
    switch (int_no) {
        case TIMER_IRQ:
            break;
        case KEYBOARD_IRQ:
            extern void keyboard_handler(void);
            keyboard_handler();
            break;
        case MOUSE_IRQ:
            extern void mouse_handler(void);
            mouse_handler();
            break;
        default:
            break;
    }
}
