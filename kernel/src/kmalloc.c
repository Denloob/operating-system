#include "kmalloc.h"
#include "brk.h"
#include "compiler_macros.h"
#include "math.h"
#include "memory.h"
#include "assert.h"
#include "mmap.h"

typedef struct malloc_chunk malloc_chunk;
typedef struct malloc_state malloc_state;

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

// Offset from malloc_chunk to the memory region accessible by user when chunk is
//  in use
#define CHUNK_HEADER_SIZE (sizeof(size_t) * 2)
#define CHUNK_PARTIAL_HEADER_SIZE (sizeof(size_t)) // We do not include the prev_chunk_size

enum {
    MALLOC_CHUNK_PREV_IN_USE    = 0x1,
    MALLOC_CHUNK_MMAPED          = 0x2,
};

#define addr2chunk(chunk) (malloc_chunk *)((void *)(chunk) - CHUNK_HEADER_SIZE)
#define chunk2addr(chunk) (void *)(&((chunk)->bk))

#define MIN_CHUNK_SIZE          (sizeof(malloc_chunk))
#define CHUNK_SIZE_ALIGN        (CHUNK_HEADER_SIZE)
#define CHUNK_OVERLAP_OFFSET    (sizeof(size_t)) // prev_chunk_size is located inside previous chunk

#define chunk_size(chunk) ((chunk)->chunk_size & (~0x7))

__attribute__((const))
malloc_chunk *next_chunk(malloc_chunk *chunk)
{
     return ((void *)(chunk) + chunk_size(chunk));
}

/*
                   Internal State Managing
*/

struct malloc_state
{
    malloc_chunk *top; // The top free chunk
};

static malloc_state main_arena;
static bool is_malloc_initialized = false;

static void malloc_initialize()
{
    is_malloc_initialized = true;

    void *allocated_addr;
    res rs = sbrk(PAGE_SIZE, &allocated_addr);
    assert(IS_OK(rs));

    main_arena.top = allocated_addr;
    main_arena.top->chunk_size = PAGE_SIZE | MALLOC_CHUNK_PREV_IN_USE;
}

/**
 * @brief - Grow the malloc heap enough, so a **chunk** of size wanted_size would
 *              fit.
 *
 * @return res_OK on success
 */
WUR static res malloc_grow_heap(size_t wanted_size)
{
    size_t top_size = main_arena.top->chunk_size;
    assert(wanted_size > top_size && "asked to grow heap when the requested size fits already");

    size_t needed_size_left = wanted_size - top_size;
    size_t size_increase = PAGE_ALIGN_UP(needed_size_left);
    res rs = sbrk(size_increase, NULL);
    if (IS_OK(rs))
    {
        size_t new_chunk_size;
        bool overflow = __builtin_add_overflow(main_arena.top->chunk_size, size_increase, &new_chunk_size);
        if (overflow)
            return res_INVALID_ARG;
    }

    return rs;
}

/*
                   kmalloc / kfree
*/

void *kmalloc(size_t size)
{
    if (!is_malloc_initialized)
        malloc_initialize();

    // TODO: if size is large enough, mmap it instead

    size_t victim_size = math_ALIGN_UP(size + CHUNK_PARTIAL_HEADER_SIZE, CHUNK_SIZE_ALIGN);

    size_t top_size = main_arena.top->chunk_size;
    if (top_size < victim_size)
    {
        res rs = malloc_grow_heap(victim_size);
        if (IS_ERR(rs))
            return NULL;
    }

    // Allocate a chunk from the top of the heap
    malloc_chunk *victim = main_arena.top;

    victim->chunk_size = victim_size | (top_size & MALLOC_CHUNK_PREV_IN_USE);
    main_arena.top = next_chunk(victim);
    main_arena.top->chunk_size = (top_size - victim_size) | MALLOC_CHUNK_PREV_IN_USE;

    return chunk2addr(victim);
}

void kfree(void *addr)
{
    malloc_chunk *chunk = addr2chunk(addr);

    assert(chunk->chunk_size > 0 && "chunk size 0");
    malloc_chunk *next = next_chunk(chunk);
    assert((next->chunk_size & MALLOC_CHUNK_PREV_IN_USE) == 1 && "next chunk doesn't think this one exists");

    // TODO: set fd and bk

    next->prev_chunk_size = chunk_size(chunk);
    next->chunk_size &= (~MALLOC_CHUNK_PREV_IN_USE);
}
