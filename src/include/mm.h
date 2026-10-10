#ifndef MM_H
#define MM_H

#include "stdint.h"

// Parenthesized so the macro is safe in expressions, not just array sizes.
#define HEAP_SIZE (1024 * 1024) // 1MB heap

void mm_init(void);
void* kmalloc(size_t size);
void kfree(void* ptr);

#endif