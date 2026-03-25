#ifndef HAL_H
#define HAL_H

// HAL layer interfaces
#include "cpu.h"
#include "memory.h"
#include "interrupts.h"
#include "io.h"

// HAL initialization
void hal_init();

#endif