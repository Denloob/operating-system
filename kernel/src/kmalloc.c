#include "kmalloc.h"
#include "memory.h"
#include "assert.h"
#include "mmap.h"

typedef struct malloc_chunk malloc_chunk;
typedef struct malloc_state malloc_state;

/*
                   Internal State Managing
*/

struct malloc_state
{
    malloc_chunk *top; // The top free chunk
};

/*
 * We will be mapping the kernel heap at a constant address, and then growing
 * it from there if we need it.
 */
#define HEAP_BEGIN (void *)0x555000000
#define MMAP(addr, size)                                                       \
    mmap(addr, size, MMAP_PROT_READ | MMAP_PROT_WRITE)

static malloc_state main_arena;
static bool is_malloc_initialized = false;

static void malloc_grow_heap()
{
    main_arena.top = (void *)PAGE_ALIGN_UP((size_t)main_arena.top);
    res rs = MMAP(main_arena.top, PAGE_SIZE);
    assert(IS_OK(rs));
}

static void malloc_initialize()
{
    main_arena.top = HEAP_BEGIN;
    is_malloc_initialized = true;
}

/*
                   Chunk Representations
*/

struct malloc_chunk
{
    size_t prev_chunk_size;        // Size of previous chunk (accessible only if prev's free).
    size_t chunk_size;             // Size of this chunk. 8 byte aligned. Lower 3 bytes used as prev chunk status

    struct malloc_chunk *bk;       // When current chunk is free, these two act as a linked list of free chunks.
    struct malloc_chunk *fd;
};

/*
                   kmalloc / kfree
*/

void *kmalloc(size_t size)
{
    if (!is_malloc_initialized)
        malloc_initialize();

    malloc_chunk *chosen = main_arena.top;
    res rs = MMAP(main_arena.top, size+sizeof(size_t)*2);
    if (IS_ERR(rs))
        return NULL;

    chosen->chunk_size = size;

    main_arena.top += size+sizeof(size_t)*2;
    main_arena.top = (void *)PAGE_ALIGN_UP((size_t)main_arena.top);
    return (void*)chosen+sizeof(size_t)*2;
}

void kfree(void *addr)
{
    malloc_chunk *chunk = addr - sizeof(size_t)*2;

    assert(chunk->chunk_size > 0 && "chunk size 0");
    munmap(chunk, chunk->chunk_size);
}
