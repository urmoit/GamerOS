#ifndef MICROKERNEL_H
#define MICROKERNEL_H

#include "scheduler.h"
#include "mm.h"
#include "ipc.h"

void microkernel_init();

void terminate_process(int pid);

#endif