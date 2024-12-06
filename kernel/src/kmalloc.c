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
#define chunk_size_of_content(chunk) (chunk_size(chunk) - CHUNK_OVERLAP_OFFSET)
#define chunk_flags(chunk) ((chunk)->chunk_size & (0x7))

size_t request_size_to_chunk_size(size_t request_size)
{
    size_t victim_size = math_ALIGN_UP(request_size + CHUNK_PARTIAL_HEADER_SIZE, CHUNK_SIZE_ALIGN);
    if (victim_size < MIN_CHUNK_SIZE)
    {
        return MIN_CHUNK_SIZE;
    }

    return victim_size;
}


__attribute__((const))
malloc_chunk *next_chunk(malloc_chunk *chunk)
{
     return ((void *)(chunk) + chunk_size(chunk));
}

__attribute__((const))
bool is_prev_free(malloc_chunk *chunk)
{
    return (chunk->chunk_size & MALLOC_CHUNK_PREV_IN_USE) == 0;
}

/**
 * @brief - Get the previous chunk.
 *          Call this function only is prev is free!
 */
__attribute__((const))
malloc_chunk *prev_chunk(malloc_chunk *chunk)
{
    assert(is_prev_free(chunk) && "Cannot get prev chunk unless it's free");
    return (void *)chunk - chunk->prev_chunk_size;
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
 * Then follow the small and large bins. In each small bin, all the chunks are
 * of the same size, the step between each small bin is 16 bytes. So in the
 * first small bin (bins[1]), all the chunks are of size 0x20 (MIN_CHUNK_SIZE), next is
 * 0x30, 0x40, .., 0x3f0, 0x400.
 * After the small bins, come the large bins, which TODO:
 *
 * bins[1..63]   - small bins (each spaced by 16 bytes in size)
 * bins[64..127] - large bins (each spaced by 4096 bytes in size)
 *
 * To save some space and logic, the bins are stored as 2 pointers in the arena,
 * but accessed as if they represented an actual chunk (using some pointer
 * repositioning tricks).
 */

#define BIN_COUNT 128
typedef malloc_chunk malloc_bin;

// Get the bin at the index `idx` in an arena.
#define bin_at(arena, idx)                                                     \
    ((malloc_bin *)(((char *)&((arena)->bins[(idx)]))                           \
                    - offsetof(malloc_chunk, fd)))
#define is_bin_empty(bin) ((bin)->fd == bin)

#define SMALL_BIN_WIDTH     CHUNK_SIZE_ALIGN
#define SMALL_BIN_COUNT     64
#define MIN_LARGE_BIN_SIZE  (SMALL_BIN_COUNT * SMALL_BIN_WIDTH)
#define is_small_bin_size(chunk_size) ((size_t)(chunk_size) < (size_t)MIN_LARGE_BIN_SIZE)
#define small_bin_index(chunk_size)   (((size_t)(chunk_size) >> 4) - 1) // Size step between each bin is 16 (aka 2**4), -index correction (0x20>>4 should map to index 1 and not 2)
#define large_bin_index(chunk_size)   (assert(false), 0) // TODO: impl

#define TCACHE_PTR_PROTECT(location, ptr) (((uint64_t)(location) >> 12) ^ (uint64_t)(ptr))

struct malloc_state
{
    malloc_chunk *top; // The top free chunk
    size_t _pad; // Padding so bins don't index out of the arena bounds
    struct {
        malloc_chunk *fd;
        malloc_chunk *bk;
    } bins[BIN_COUNT];
};

/**
 * @brief Checks if the given chunk (which is not `top`) is free.
 */
__attribute__((const))
bool is_chunk_free(malloc_chunk *chunk)
{
    malloc_chunk *next = next_chunk(chunk);
    return (next->chunk_size & MALLOC_CHUNK_PREV_IN_USE) == 0;
}

void bin_insert(malloc_bin *bin, malloc_chunk *chunk)
{
    chunk->fd = bin->fd;
    chunk->bk = bin;
    bin->fd->bk = chunk; // On first insert, when bin->fd == bin, will set bin->bk to be `chunk`
    bin->fd = chunk;
}

static malloc_state main_arena;
static bool is_malloc_initialized = false;

static void initialize_bins()
{
    // Initialize circular links for the bins
    for (int i = 0; i < BIN_COUNT; i++)
    {
        malloc_bin *bin = bin_at(&main_arena, i);
        bin->fd = bin->bk = bin;
    }
}

static void initialize_top_chunk()
{
    void *allocated_addr;
    res rs = sbrk(PAGE_SIZE, &allocated_addr);
    assert(IS_OK(rs));

    main_arena.top = allocated_addr;
    main_arena.top->chunk_size = PAGE_SIZE | MALLOC_CHUNK_PREV_IN_USE;

}

static void malloc_initialize()
{
    is_malloc_initialized = true;

    initialize_top_chunk();

    initialize_bins();
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

/**
 * @brief - Check if the heap is big enough to put a chunk of the end of it.
 *
 * @param chunk_size - The size of the chunk to test for
 * @return - if it would fit
 */
__attribute__((const))
bool is_heap_big_enough_for_chunk(size_t chunk_size)
{
    return chunk_size <= main_arena.top->chunk_size;
}

/**
 * @brief - Splits a given chunk in two. The first part is of size `wanted_size`
 *              and the other one the rest of it.
 *              In the end of the function, the first part is located at
 *              `chunk_to_split`, and a pointer to the other part is returned.
 *
 * @param chunk_to_split - The chunk you want to split
 * @param wanted_size    - The size of the first part of the split chunk
 *
 * @return - Pointer to the second part of the split (first part == `chunk_to_split`).
 */
malloc_chunk *split_chunk_into_two(malloc_chunk *chunk_to_split, size_t wanted_size)
{
    size_t original_size = chunk_to_split->chunk_size;
    size_t original_flags = original_size & MALLOC_CHUNK_PREV_IN_USE;

    malloc_chunk *part1 = chunk_to_split;
    part1->chunk_size = wanted_size | original_flags;

    malloc_chunk *part2 = next_chunk(part1);
    part2->chunk_size = original_size - wanted_size;
    part2->chunk_size |= MALLOC_CHUNK_PREV_IN_USE;

    return part2; // part1 is at `chunk_to_split`, so no need to return it
}

/**
 * @brief - Allocate the chunk from the top of the heap, aka the free space on the
 *              heap which was never allocated before. This is used only as a last
 *              resort, when there's no other way to reuse previously freed chunks.
 *
 * @param victim_size - The size of the **chunk** (not the user request)
 *                          that needs allocating
 * @return - Pointer to the allocated memory (not chunk)
 */
void *malloc_from_top(size_t victim_size)
{
    if (!is_heap_big_enough_for_chunk(victim_size))
    {
        res rs = malloc_grow_heap(victim_size);
        if (IS_ERR(rs))
            return NULL;
    }

    malloc_chunk *old_top = main_arena.top;
    main_arena.top = split_chunk_into_two(old_top, victim_size);

    return chunk2addr(old_top);
}

/**
 * @brief Sort the given chunk, which is not already in a bin, into small/large
 *          bin.
 *
 * @param chunk The chunk to sort. Must not be in any other bin.
 */
void sort_chunk_into_bins(malloc_chunk *chunk)
{
    size_t size = chunk_size(chunk);
    if (is_small_bin_size(size))
    {
        malloc_bin *bin = bin_at(&main_arena, small_bin_index(size));
        bin_insert(bin, chunk);
        return;
    }

    // TODO: find into what bin, and then insert at the correct thing (large bin must be sorted by size)
    assert(false);
}

/**
 * @brief Malloc from the unsorted bin, if possible. While going through the
 *          unsorted bin, will sort all entries inside which do not match the
 *          request into small and large bins.
 *
 * @param victim_size The size of the **chunk** (not the user request)
 *                        that needs allocating
 * @return - Pointer to the allocated memory (not chunk) or NULL.
 */
void *malloc_from_unsorted_bin(size_t victim_size)
{
    malloc_bin *bin = bin_at(&main_arena, 0);
    malloc_chunk *cur = bin->fd;

    while (cur != bin)
    {
        malloc_chunk *next_in_bin = cur->fd;
        unlink_chunk(cur);

        if (victim_size <= chunk_size(cur))
        {
            next_chunk(cur)->chunk_size |= MALLOC_CHUNK_PREV_IN_USE;

            // TODO: if victim_size is smaller than the chunk, split.

            return chunk2addr(cur);
        }

        sort_chunk_into_bins(cur);

        cur = next_in_bin;
    }

    return NULL;
}

/**
 * @brief - Malloc from one of the small bins, if possible.
 *
 * @param - victim_size The size of the **chunk** (not the user request)
 *                        that needs allocating
 * @return - Pointer to the allocated memory (not chunk) or NULL.
 */
void *malloc_from_small_bin(size_t victim_size)
{
    if (!is_small_bin_size(victim_size))
    {
        return NULL;
    }

    malloc_bin *bin = bin_at(&main_arena, small_bin_index(victim_size));
    if (is_bin_empty(bin))
    {
        return NULL;
    }

    malloc_chunk *chunk = bin->bk;
    unlink_chunk(chunk);

    next_chunk(chunk)->chunk_size |= MALLOC_CHUNK_PREV_IN_USE;
    return chunk2addr(chunk);
}

/**
 * @brief - Malloc from one of the small bins, if possible.
 *
 * @param - victim_size The size of the **chunk** (not the user request)
 *                        that needs allocating
 * @return - Pointer to the allocated memory (not chunk) or NULL.
 */
void *malloc_from_large_bin(size_t victim_size)
{
    // next_chunk(chunk)->chunk_size |= MALLOC_CHUNK_PREV_IN_USE;
    return NULL; // TODO: implement large bin allocation
}

/**
 * @brief - Malloc from one of the bins, if possible. If no chunk is found in
 *          small/large bins, will go through unsorted bin, sorting it while
 *          searching.
 * @see   - malloc_from_unsorted_bin
 *
 * @param - victim_size The size of the **chunk** (not the user request)
 *                        that needs allocating
 * @return - Pointer to the allocated memory (not chunk) or NULL.
 */
void *malloc_from_bins(size_t victim_size)
{
    void *mem = malloc_from_small_bin(victim_size);
    if (mem) return mem;

    mem = malloc_from_large_bin(victim_size);
    if (mem) return mem;

    return malloc_from_unsorted_bin(victim_size);
}

/**
 * @brief Combine two consecutive **free** chunks, where first is right before
 *          the second one. That is, next_chunk(first) == second.
 *          Additionally, `second` may not be the top chunk.
 */
void combine_free_chunks(malloc_chunk *first, malloc_chunk *second)
{
    assert(next_chunk(first) == second);
    assert(is_chunk_free(first) && is_chunk_free(second));

    size_t new_size = chunk_size(first) + chunk_size(second);
    size_t flags_of_first = chunk_flags(first);

    first->chunk_size = new_size | flags_of_first;

    malloc_chunk *next = next_chunk(second);
    next->prev_chunk_size = new_size;
}

/**
 * @brief Combine the given chunk with the top of the heap.
 *          The chunk must be right before the top, that is
 *          next_chunk(chunk) == arena->top.
 */
void combine_chunk_with_top(malloc_chunk *chunk, malloc_state *arena)
{
    assert(next_chunk(chunk) == arena->top);

    size_t new_top_size = chunk_size(chunk) + chunk_size(arena->top);

    chunk->chunk_size = new_top_size | chunk_flags(chunk);
    arena->top = chunk;
}

/**
 * @brief - Consolidates (merges) the given chunk with the previous chunk, if it's free.
 *
 * @param chunk - The chunk to try to consolidate with previous chunk
 *
 * @return - Address of the consolidated chunk.
 */
malloc_chunk *try_consolidate_previous(malloc_chunk *chunk)
{
    if (!is_prev_free(chunk)) return chunk;

    malloc_chunk *prev = prev_chunk(chunk);

    unlink_chunk(chunk);

    combine_free_chunks(prev, chunk);
    return prev;
}

/**
 * @brief - Consolidates (merges) the given chunk with the next chunk, if it's free.
 *
 * @param chunk - The chunk to try to consolidate with next chunk
 */
void try_consolidate_next(malloc_chunk *chunk, malloc_state *arena)
{
    malloc_chunk *next = next_chunk(chunk);

    if (next == arena->top)
    {
        unlink_chunk(chunk);
        combine_chunk_with_top(chunk, arena);
        return;
    }

    // We must check this only when we are sure `next != arena->top`.
    if (!is_chunk_free(next)) return;

    unlink_chunk(next);
    combine_free_chunks(chunk, next);
}

/**
 * @brief Try to consolidate the given chunk with the next/previous one.
 *          Consolidates if possible, and does nothing otherwise.
 *
 * @param chunk The chunk to try to consolidate with the next/prev chunk.
 */
void try_consolidate(malloc_chunk *chunk, malloc_state *arena)
{
    // NOTE: it's important we first try to consolidate with prev, because a successful consolidation with top would result in `chunk` no longer being a normal chunk
    chunk = try_consolidate_previous(chunk);
    try_consolidate_next(chunk, arena);
}

/*
                   kmalloc / kfree
*/

void *kmalloc(size_t size)
{
    if (!is_malloc_initialized)
        malloc_initialize();

    // TODO: if size is large enough, mmap it instead

    size_t victim_size = request_size_to_chunk_size(size);

    void *victim = malloc_from_bins(victim_size);
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

    malloc_bin *unsorted_bin = bin_at(&main_arena, 0);

    bin_insert(unsorted_bin, chunk);

    try_consolidate(chunk, &main_arena);
}


void *kcalloc(size_t amount, size_t size)
{
    size_t total_size;
    bool overflow = __builtin_mul_overflow(amount, size, &total_size);
    if (overflow)
    {
        return NULL;
    }

    void *p = kmalloc(total_size);
    if (p == NULL)
    {
        return NULL;
    }

    memset(p, 0, total_size);
    return p;
}

void *krealloc(void *ptr, size_t size)
{
    if (ptr == NULL)
    {
        return kmalloc(size);
    }

    if (size == 0)
    {
        kfree(ptr);
        return NULL;
    }

    malloc_chunk *chunk = addr2chunk(ptr);

    size_t required_chunk_size = request_size_to_chunk_size(size);
    size_t current_chunk_size = chunk_size(chunk);
    if (required_chunk_size <= current_chunk_size)
    {
        return ptr;
    }

    void *new_ptr = kmalloc(size);
    if (new_ptr == NULL)
    {
        return NULL;
    }

    memmove(new_ptr, ptr, chunk_size_of_content(chunk));
    kfree(ptr);

    return new_ptr;
}

// GCC optimizes kmalloc and causes it to not know that it returns a pointer to
//  be freed by kfree, causing this warning.
#pragma GCC diagnostic push
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic ignored "-Wmismatched-dealloc"
#endif
void test_kmalloc()
{
    // Malloc correctly gets the size
    void *addr = kmalloc(1);
    void *pad = kmalloc(1);
    malloc_chunk *chunk_addr = addr2chunk(addr);
    assert(chunk_size(chunk_addr) == MIN_CHUNK_SIZE);


    // Free puts the chunk into the unsorted bin
    kfree(addr);
    assert(bin_at(&main_arena, 0)->fd == chunk_addr);

    // Malloc sorts the unsorted bin
    void *addr2 = kmalloc(MIN_CHUNK_SIZE * 2);
    assert(bin_at(&main_arena, small_bin_index(MIN_CHUNK_SIZE))->fd == chunk_addr);

    // Allocate the chunk from the small bin
    void *addr3 = kmalloc(1);
    assert(addr3 == addr);

    // Free all
    kfree(addr3);
    kfree(addr2);
    kfree(pad);

    // Malloc consolidates all previous allocations
    addr2 = kmalloc(MIN_CHUNK_SIZE * 4);
    assert(addr2 == addr);
    kfree(addr2);
}
#pragma GCC diagnostic pop
