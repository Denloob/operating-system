#include "brk.h"
#include "memory.h"
#include "mmap.h"
#include "res.h"
#include <stdint.h>

// Page break is the limit of the allocated memory region of the kernel.
//  i.e. moving it by 5 pages means allocating 5 pages consecutively,
//          with each next move being more consecutive mappings.
//  useful for allocations where the virtual address doesn't matter,
//      such as malloc.
//  There's only one page break for the kernel.
#define DEFAULT_PAGE_BREAK 0xffffa110c0000000
uint64_t g_page_break = DEFAULT_PAGE_BREAK;

res kbrk(void *addr_in)
{
    return brk(addr_in, &g_page_break, MMAP_PROT_READ | MMAP_PROT_WRITE);
}

res ksbrk(int64_t increment, void **prev_brk)
{
    return sbrk(increment, prev_brk, &g_page_break, MMAP_PROT_READ | MMAP_PROT_WRITE);
}

res brk(void *addr_in, uint64_t *page_break_state, mmap_Protection prot)
{
    uint64_t addr = (uint64_t)addr_in;
    uint64_t addr_aligned = PAGE_ALIGN_UP(addr);

    // We will allocate page-by-page, but allow non-aligned page break state.
    //  so this is the "actual" page break
    uint64_t page_break_aligned = PAGE_ALIGN_UP(*page_break_state);

    if (page_break_aligned == addr_aligned)
    {
        *page_break_state = addr;
        return res_OK;
    }

    bool should_unmap = addr_aligned < page_break_aligned;
    if (should_unmap)
    {
        uint64_t unmap_size = page_break_aligned - addr_aligned;
        munmap((void *)addr_aligned, unmap_size);
        *page_break_state = addr;

        return res_OK;
    }

    uint64_t map_size = addr_aligned - page_break_aligned;
    res rs = mmap((void *)page_break_aligned, map_size, prot);
    if (IS_OK(rs))
    {
        *page_break_state = addr;
    }
    return rs;
}

res sbrk(int64_t increment, void **prev_brk, uint64_t *page_break_state, mmap_Protection prot)
{
    if (prev_brk != NULL)
    {
        *prev_brk = (void *)*page_break_state;
    }

    if (increment == 0)
        return res_OK;

    size_t new_break;
    if (__builtin_add_overflow(*page_break_state, increment, &new_break))
        return res_brk_CHANGE_TO_LARGE;

    return brk((void *)new_break, page_break_state, prot);
}
