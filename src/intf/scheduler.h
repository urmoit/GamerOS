#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "stdint.h"

#define MAX_PROCESSES 16
#define STACK_SIZE 4096

enum process_state {
    PROCESS_RUNNING,
    PROCESS_READY,
    PROCESS_BLOCKED,
    PROCESS_TERMINATED
};

typedef struct {
    uint32_t pid;
    uint64_t rsp; // Stack pointer
    uint64_t rbp; // Base pointer
    enum process_state state;
    uint8_t stack[STACK_SIZE];
} pcb_t;

void scheduler_init();
void create_process(void (*entry_point)());
void schedule();

#endif