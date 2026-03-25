#ifndef MICROKERNEL_H
#define MICROKERNEL_H

// Microkernel layer interfaces
#include "scheduler.h"
#include "mm.h"
#include "ipc.h"

// Microkernel initialization
void microkernel_init();

// Process management
void terminate_process(int pid);

#endif