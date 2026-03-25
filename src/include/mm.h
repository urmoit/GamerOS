#ifndef MM_H
#define MM_H

#include "stdint.h"

#define HEAP_SIZE 1024 * 1024 // 1MB heap

void mm_init();
void* kmalloc(size_t size);
void kfree(void* ptr);

#endif