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

    struct malloc_chunk *fd;       // When current chunk is free, these two act as a linked list of free chunks.
    struct malloc_chunk *bk;
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
#define chunk2addr(chunk) (void *)(&((chunk)->fd))

#define MIN_CHUNK_SIZE          (sizeof(malloc_chunk))
#define CHUNK_SIZE_ALIGN        (CHUNK_HEADER_SIZE)
#define CHUNK_OVERLAP_OFFSET    (sizeof(size_t)) // prev_chunk_size is located inside previous chunk

#define chunk_size(chunk) ((chunk)->chunk_size & (~0x7))

__attribute__((const))
malloc_chunk *next_chunk(malloc_chunk *chunk)
{
     return ((void *)(chunk) + chunk_size(chunk));
}

// Remove `chunk` from a linked list via its `fd` and `bk`.
void unlink_chunk(malloc_chunk *chunk)
{
    assert(chunk->fd->bk == chunk && chunk->bk->fd == chunk && "chunk not in a valid linked list");
    malloc_chunk *fd = chunk->fd;
    malloc_chunk *bk = chunk->bk;

    chunk->bk->fd = fd;
    chunk->fd->bk = bk;
}

/*
                   Internal State Managing
*/


/* Bins
 * ----
 * Each bin in an arena represents a doubly linked list of free chunks of a
 * particular size range. That is, if a chunk of a specific size is needed,
 * a corresponding bin can be used to quickly get an address that is currently
 * not in use.
 *
 * The first bin is an "unsorted bin", i.e. it contains chunks of all sizes.
 * these chunks are placed there by `free` function, and later malloc will pop
 * them, and either use them if they match the request, or sort them into the
 * correct bin.
 *
 * To save some space and logic, the bins are stored as 2 pointers in the arena,
 * but accessed as if they represented an actual chunk (using some pointer
 * repositioning tricks).
 */

#define BIN_COUNT 1
typedef malloc_chunk malloc_bin;

// Get the bin at the index `idx` in an arena.
#define bin_at(arena, idx)                                                     \
    (malloc_bin *)(((char *)&((arena)->bins[(idx)]))                           \
                    - offsetof(malloc_chunk, fd))
// TODO: support more bins than just unsorted bin

struct malloc_state
{
    malloc_chunk *top; // The top free chunk
    size_t _pad; // Padding so bins don't index out of the arena bounds
    struct {
        malloc_chunk *fd;
        malloc_chunk *bk;
    } bins[BIN_COUNT];
};

void bin_insert(malloc_bin *bin, malloc_chunk *chunk)
{
    chunk->fd = bin->fd;
    chunk->bk = bin;
    bin->fd->bk = chunk; // On first insert, when bin->fd == bin, will set bin->bk to be `chunk`
    bin->fd = chunk;
}

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

    // Initialize circular links for the bins
    for (int i = 0; i < BIN_COUNT; i++)
    {
        malloc_bin *bin = bin_at(&main_arena, i);
        bin->fd = bin->bk = bin;
    }
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

void *malloc_from_top(size_t victim_size)
{
    size_t top_size = main_arena.top->chunk_size;
    if (top_size < victim_size)
    {
        res rs = malloc_grow_heap(victim_size);
        if (IS_ERR(rs))
            return NULL;
    }

    malloc_chunk *victim = main_arena.top;

    victim->chunk_size = victim_size | (top_size & MALLOC_CHUNK_PREV_IN_USE);
    main_arena.top = next_chunk(victim);
    main_arena.top->chunk_size = (top_size - victim_size) | MALLOC_CHUNK_PREV_IN_USE;

    return chunk2addr(victim);
}

void *malloc_from_bin(size_t victim_size)
{
    malloc_bin *bin = bin_at(&main_arena, 0);
    malloc_chunk *cur = bin->fd;

    while (cur != bin)
    {
        if (chunk_size(cur) <= victim_size)
        {
            unlink_chunk(cur);

            malloc_chunk *next = next_chunk(cur);
            next->chunk_size |= MALLOC_CHUNK_PREV_IN_USE;

            return cur;
        }
    }

    return NULL;
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
    if (victim_size < MIN_CHUNK_SIZE) victim_size = MIN_CHUNK_SIZE;

    void *victim = malloc_from_bin(victim_size);
    if (victim != NULL)
    {
        return victim;
    }

    return malloc_from_top(victim_size);
}

void kfree(void *addr)
{
    malloc_chunk *chunk = addr2chunk(addr);

    assert(chunk->chunk_size > 0 && "chunk size 0");
    malloc_chunk *next = next_chunk(chunk);
    assert((next->chunk_size & MALLOC_CHUNK_PREV_IN_USE) == 1 && "next chunk doesn't think this one exists");

    next->prev_chunk_size = chunk_size(chunk);
    next->chunk_size &= (~MALLOC_CHUNK_PREV_IN_USE);

    // TODO: consolidate if possible

    malloc_bin *unsorted_bin = bin_at(&main_arena, 0);

    bin_insert(unsorted_bin, chunk);
}
