#include "../../intf/scheduler.h"
#include "../../intf/stdint.h"
#include "../../intf/string.h"

static pcb_t processes[MAX_PROCESSES];
static int current_process = -1;
static int next_pid = 0;
static int scheduler_lock = 0; // Simple spinlock for scheduler operations

extern void switch_context(uint64_t old_rsp, uint64_t new_rsp);

void scheduler_init() {
    for (size_t i = 0; i < MAX_PROCESSES; i++) {
        processes[i].pid = 0;
        processes[i].state = PROCESS_TERMINATED;
        processes[i].rsp = 0;
        processes[i].rbp = 0;
    }
    scheduler_lock = 0; // Initialize lock
}

void create_process(void (*entry_point)()) {
    if (!entry_point) return; // Error recovery: NULL entry point

    // Acquire scheduler lock
    while (__atomic_test_and_set(&scheduler_lock, __ATOMIC_ACQUIRE)) {
        // Spin until lock is acquired
    }

    if (next_pid >= MAX_PROCESSES) {
        __atomic_clear(&scheduler_lock, __ATOMIC_RELEASE);
        return; // Max processes reached - graceful failure
    }

    pcb_t* new_pcb = &processes[next_pid];
    new_pcb->pid = next_pid;
    new_pcb->state = PROCESS_READY;

    // Set up initial stack frame for 64-bit x86_64 ABI
    uint64_t* stack_ptr = (uint64_t*)(new_pcb->stack + STACK_SIZE - 8); // 8-byte alignment

    // Set up the stack as if we were returning from an interrupt
    // This matches the context switching assembly
    *(--stack_ptr) = (uint64_t)0x202; // RFLAGS (Interrupts enabled)
    *(--stack_ptr) = (uint64_t)0x8;   // CS (kernel code segment)
    *(--stack_ptr) = (uint64_t)entry_point; // RIP (instruction pointer)

    // Push all general-purpose registers (as saved by context switch)
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

    // Release scheduler lock
    __atomic_clear(&scheduler_lock, __ATOMIC_RELEASE);
}

// Function to terminate a process cleanly
void terminate_process(int pid) {
    if (pid < 0 || pid >= next_pid) return; // Invalid PID

    // Acquire scheduler lock
    while (__atomic_test_and_set(&scheduler_lock, __ATOMIC_ACQUIRE)) {
        // Spin until lock is acquired
    }

    if (processes[pid].state != PROCESS_TERMINATED) {
        processes[pid].state = PROCESS_TERMINATED;

        // If terminating current process, schedule next one
        if (current_process == pid) {
            // Find next ready process
            int next_proc = (current_process + 1) % next_pid;
            while (next_proc != current_process) {
                if (processes[next_proc].state == PROCESS_READY) {
                    current_process = next_proc;
                    break;
                }
                next_proc = (next_proc + 1) % next_pid;
            }
        }
    }

    // Release scheduler lock
    __atomic_clear(&scheduler_lock, __ATOMIC_RELEASE);
}

void schedule() {
    // Acquire scheduler lock
    while (__atomic_test_and_set(&scheduler_lock, __ATOMIC_ACQUIRE)) {
        // Spin until lock is acquired
    }

    if (current_process == -1 || next_pid <= 1) {
        __atomic_clear(&scheduler_lock, __ATOMIC_RELEASE);
        return;
    }

    int next_proc = current_process;
    do {
        next_proc = (next_proc + 1) % next_pid;
        if (processes[next_proc].state == PROCESS_READY) {
            if (next_proc != current_process) {
                uint64_t old_rsp = processes[current_process].rsp;
                current_process = next_proc;
                // Release lock before context switch
                __atomic_clear(&scheduler_lock, __ATOMIC_RELEASE);
                switch_context(old_rsp, processes[current_process].rsp);
                return;
            }
            break;
        }
    } while (next_proc != current_process);

    // Release scheduler lock
    __atomic_clear(&scheduler_lock, __ATOMIC_RELEASE);
}