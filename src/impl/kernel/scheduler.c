#include "../../intf/scheduler.h"
#include "../../intf/stdint.h"
#include "../../intf/string.h"

static pcb_t processes[MAX_PROCESSES];
static int current_process = -1;
static int next_pid = 0;

extern void switch_context(uint64_t old_rsp, uint64_t new_rsp);

void scheduler_init() {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        processes[i].pid = 0;
        processes[i].state = PROCESS_TERMINATED;
    }
}

void create_process(void (*entry_point)()) {
    if (next_pid >= MAX_PROCESSES) {
        return; // Max processes reached
    }

    pcb_t* new_pcb = &processes[next_pid];
    new_pcb->pid = next_pid;
    new_pcb->state = PROCESS_READY;

    // Set up initial stack frame for 64-bit
    uint64_t* stack_ptr = (uint64_t*)(new_pcb->stack + STACK_SIZE - 8); // 8-byte alignment

    *(--stack_ptr) = (uint64_t)0x202; // RFLAGS (Interrupts enabled)
    *(--stack_ptr) = (uint64_t)0x8;   // CS
    *(--stack_ptr) = (uint64_t)entry_point; // RIP

    // Push dummy registers
    *(--stack_ptr) = 0; // RAX
    *(--stack_ptr) = 0; // RBX
    *(--stack_ptr) = 0; // RCX
    *(--stack_ptr) = 0; // RDX
    *(--stack_ptr) = 0; // RBP
    *(--stack_ptr) = 0; // RSI
    *(--stack_ptr) = 0; // RDI
    *(--stack_ptr) = 0; // R8
    *(--stack_ptr) = 0; // R9
    *(--stack_ptr) = 0; // R10
    *(--stack_ptr) = 0; // R11
    *(--stack_ptr) = 0; // R12
    *(--stack_ptr) = 0; // R13
    *(--stack_ptr) = 0; // R14
    *(--stack_ptr) = 0; // R15

    new_pcb->rsp = (uint64_t)stack_ptr;

    if (current_process == -1) {
        current_process = next_pid;
    }
    next_pid++;
}

void schedule() {
    if (current_process == -1) return;

    int next_proc = current_process;
    do {
        next_proc = (next_proc + 1) % next_pid;
        if (processes[next_proc].state == PROCESS_READY) {
            if (next_proc != current_process) {
                uint64_t old_rsp = processes[current_process].rsp;
                current_process = next_proc;
                switch_context(old_rsp, processes[current_process].rsp);
            }
            return;
        }
    } while (next_proc != current_process);
}