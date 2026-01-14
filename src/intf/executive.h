#ifndef EXECUTIVE_H
#define EXECUTIVE_H

#include "stdint.h"

// Executive Layer initialization
void executive_init(void);

// Executive Services initialization functions
void object_manager_init(void);
void filesystem_manager_init(void);
void io_manager_init(void);

// Placeholder for other executive services
// void process_manager_init(void);
// void security_manager_init(void);
// void power_manager_init(void);
// void vmm_init(void);

#endif