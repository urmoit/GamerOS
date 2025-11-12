#include "../../intf/mm.h"
#include "../../intf/stdint.h"
#include "../../intf/string.h"

// Simple heap implementation with basic free list
#define BLOCK_SIZE sizeof(block_t)
#define MIN_BLOCK_SIZE 16

typedef struct block {
    size_t size;
    int free;
    struct block* next;
} block_t;

static uint8_t heap[HEAP_SIZE];
static block_t* free_list = 0;

void mm_init() {
    memset(heap, 0, HEAP_SIZE);
    free_list = (block_t*)heap;
    free_list->size = HEAP_SIZE - BLOCK_SIZE;
    free_list->free = 1;
    free_list->next = 0;
}

void* kmalloc(size_t size) {
    // Ensure proper alignment for all data types (16-byte alignment for SIMD)
    size = (size + 15) & ~15; // 16-byte alignment
    size_t total_size = size + BLOCK_SIZE;

    block_t* current = free_list;
    block_t* prev = 0;

    while (current) {
        if (current->free && current->size >= size) {
            // Found a suitable block
            if (current->size >= size + BLOCK_SIZE + MIN_BLOCK_SIZE) {
                // Split the block
                block_t* new_block = (block_t*)((uint8_t*)current + BLOCK_SIZE + size);
                new_block->size = current->size - size - BLOCK_SIZE;
                new_block->free = 1;
                new_block->next = current->next;

                current->size = size;
                current->next = new_block;
            }

            current->free = 0;
            return (void*)((uint8_t*)current + BLOCK_SIZE);
        }
        prev = current;
        current = current->next;
    }

    return 0; // Out of memory
}

void kfree(void* ptr) {
    if (!ptr) return;

    block_t* block = (block_t*)((uint8_t*)ptr - BLOCK_SIZE);
    block->free = 1;

    // Simple coalescing: merge with next block if also free
    if (block->next && block->next->free) {
        block->size += block->next->size + BLOCK_SIZE;
        block->next = block->next->next;
    }
}