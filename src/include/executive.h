#ifndef EXECUTIVE_H
#define EXECUTIVE_H

#include "stdint.h"

// Executive Layer initialization
void executive_init(void);
int executive_is_ready(void);
int executive_get_last_error(void);

#define EXECUTIVE_ERR_NONE              0
#define EXECUTIVE_ERR_GDI_INIT          1
#define EXECUTIVE_ERR_DRIVER_REGISTER   2
#define EXECUTIVE_ERR_FS_BOOTSTRAP      3

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
