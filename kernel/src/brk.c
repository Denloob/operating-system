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
#define DEFAULT_PAGE_BREAK 0xa110c000000
uint64_t g_page_break = DEFAULT_PAGE_BREAK;

res brk(void *addr_in)
{
    uint64_t addr = (uint64_t)addr_in;
    uint64_t addr_aligned = PAGE_ALIGN_UP(addr);

    // We will allocate page-by-page, but allow non-aligned g_page_break.
    //  so this is the "actual" page break
    uint64_t page_break_aligned = PAGE_ALIGN_UP(g_page_break);

    if (page_break_aligned == addr_aligned)
    {
        g_page_break = addr;
        return res_OK;
    }

    bool should_unmap = addr_aligned < page_break_aligned;
    if (should_unmap)
    {
        uint64_t unmap_size = page_break_aligned - addr_aligned;
        munmap((void *)addr_aligned, unmap_size);
        g_page_break = addr;

        return res_OK;
    }

    uint64_t map_size = addr_aligned - page_break_aligned;
    res rs = mmap((void *)page_break_aligned, map_size, MMAP_PROT_READ | MMAP_PROT_WRITE);
    if (IS_OK(rs))
    {
        g_page_break = addr;
    }
    return rs;
}

res sbrk(int64_t increment, void **prev_brk)
{
    if (prev_brk != NULL)
    {
        *prev_brk = (void *)g_page_break;
    }

    if (increment == 0)
        return res_OK;

    size_t new_break;
    if (__builtin_add_overflow(g_page_break, increment, &new_break))
        return res_brk_CHANGE_TO_LARGE;

    return brk((void *)new_break);
}
