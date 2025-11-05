#include "../../intf/mm.h"
#include "../../intf/stdint.h"
#include "../../intf/string.h"

// Simple heap implementation
static uint8_t heap[HEAP_SIZE];
static size_t next_free_byte = 0;

void mm_init() {
    memset(heap, 0, HEAP_SIZE);
    next_free_byte = 0;
}

void* kmalloc(size_t size) {
    if (next_free_byte + size > HEAP_SIZE) {
        return 0; // Out of memory
    }
    void* ptr = &heap[next_free_byte];
    next_free_byte += size;
    return ptr;
}

void kfree(void* ptr) {
    // With this simple allocator, kfree does nothing
    // A more complex allocator would mark memory as free
}