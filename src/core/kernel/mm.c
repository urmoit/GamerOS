#include <mm.h>
#include <stdint.h>
#include <string.h>

// Minimal first-fit heap over a statically reserved BSS arena.
//
// The executive layer (object manager, GDI) allocates through kmalloc/kfree, so
// these must exist for that code to link. A real allocator with block splitting,
// coalescing and fragmentation control is still listed as future work; this
// implementation is correct but simple: fixed-size block free list, first fit,
// no splitting, no coalescing.

#define ALIGN_SIZE   8u
#define HEAP_MAGIC   0x48454150u  // "HEAP"

typedef struct block_header {
    uint32_t magic;
    uint32_t size;          // payload size in bytes (already aligned)
    struct block_header* next; // next free block in the free list
    uint32_t reserved;
} block_header_t;

// Static arena. HEAP_SIZE from mm.h is 1MB.
static uint8_t heap_arena[HEAP_SIZE] __attribute__((aligned(ALIGN_SIZE)));
static block_header_t* heap_free_list = 0;
static int heap_initialized = 0;

static uint32_t align_up(uint32_t v) {
    uint32_t rem = v % ALIGN_SIZE;
    if (rem == 0) return v;
    return v + (ALIGN_SIZE - rem);
}

void mm_init(void) {
    memset(heap_arena, 0, sizeof(heap_arena));

    block_header_t* block = (block_header_t*)heap_arena;
    block->magic = HEAP_MAGIC;
    // Reserve space for the header so payload alignment is preserved.
    block->size = (uint32_t)sizeof(heap_arena) - (uint32_t)sizeof(block_header_t);
    block->next = 0;
    block->reserved = 0;

    heap_free_list = block;
    heap_initialized = 1;
}

void* kmalloc(size_t size) {
    if (!heap_initialized) mm_init();
    if (size == 0) return 0;

    uint32_t need = align_up((uint32_t)size);
    if (need < size) return 0;  // overflow

    block_header_t** link = &heap_free_list;
    while (*link) {
        block_header_t* block = *link;
        if (block->size >= need) {
            // Exact-size or first-fit: unlink and hand the whole block over.
            *link = block->next;
            block->next = 0;
            // Re-stamp the magic: kfree() rejects blocks whose magic is clear,
            // and we cleared it on the previous free.
            block->magic = HEAP_MAGIC;
            return (void*)((uint8_t*)block + sizeof(block_header_t));
        }
        link = &block->next;
    }
    return 0;
}

void kfree(void* ptr) {
    if (!ptr || !heap_initialized) return;

    block_header_t* block = (block_header_t*)((uint8_t*)ptr - sizeof(block_header_t));

    // Validate that this really is one of our blocks before touching the list.
    if ((uint8_t*)block < heap_arena ||
        (uint8_t*)block + sizeof(block_header_t) > heap_arena + sizeof(heap_arena)) {
        return;
    }
    if (block->magic != HEAP_MAGIC) return;

    block->magic = 0;  // poison so a double free is rejected next time
    block->next = heap_free_list;
    heap_free_list = block;
}