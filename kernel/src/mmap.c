#include "kernel_memory_info.h"
#include "io.h"
#include "memory.h"
#include "mmap.h"
#include "range.h"
#include "assert.h"
#include "mmu.h"
#include "math.h"
#include "res.h"

#define PAGE_SIZE 0x1000
#define PAGE_ALIGN_UP(address)   math_ALIGN_UP(address, PAGE_SIZE)
#define PAGE_ALIGN_DOWN(address) math_ALIGN_DOWN(address, PAGE_SIZE)

#define MEMORY_MAP_MAX_LENGTH (0x1000 / sizeof(range_Range)) // NOTE: this has to be synced with the define in the bootloader (same macro name).
struct {
    range_Range *base;
    uint64_t length;
} g_phys_memory_map = {0}; // Init with 0 so it's placed in .data and not in .bss

void mmap_init(range_Range *mmap_base, uint64_t length)
{
    assert(length <= MEMORY_MAP_MAX_LENGTH && "length larger than one page is not supported\n");
    g_phys_memory_map.base = mmap_base;
    g_phys_memory_map.length = length;

    res rs = mmap((void *)KERNEL_MEMORY_MAP, PAGE_SIZE, MMAP_PROT_READ | MMAP_PROT_WRITE);
    assert(IS_OK(rs));

    memmove((void *)KERNEL_MEMORY_MAP, mmap_base, length * sizeof(range_Range));
    g_phys_memory_map.base = (void *)KERNEL_MEMORY_MAP;

    munmap(mmap_base, PAGE_SIZE);
}

// WARNING: this invalidates all g_phys_memory_map.base iterators and moves indexes
void mmap_phys_memory_add(const range_Range *range)
{
    uint64_t *const len = &g_phys_memory_map.length;
    range_Range *const base = g_phys_memory_map.base;

    assert(!(*len > MEMORY_MAP_MAX_LENGTH) && "How did len become larger than allowed?");
    if (*len == MEMORY_MAP_MAX_LENGTH)
    {
        *len = range_defragment(base, *len);
    }

    assert(*len < MEMORY_MAP_MAX_LENGTH && "Ran out of memory for the physical memory map. Dynamic memory map is unsupported"); // TODO: support resizing the memory map

    base[*len] = *range;
    (*len)++;
}

bool mmap_allocate_contiguous(uint64_t want_size, uint64_t *out_result)
{
    assert(g_phys_memory_map.base);

    return range_pop_of_size(g_phys_memory_map.base, g_phys_memory_map.length,
                                     want_size, out_result);
}

/**
 * @brief - Allocate physical memory of size `0 < result_size < want_size`, page
 *              aligned.
 *
 * @param want_size            - The size you want to allocate. The allocated
 *                                  memory will never be larger than this.
 *                                  Must be page aligned
 * @param out_result[out]      - The start address of the allocated physical memory.
 * @param out_result_size[out] - The size of the allocated physical memory.
 *
 * @return - true on success, false otherwise. If the function didn't succeed,
 *              this means that there's no more free physical RAM available of
 *              any size.
 */
bool allocate_physical_memory(uint64_t want_size, uint64_t *out_result, uint64_t *out_result_size)
{
    assert(g_phys_memory_map.base);

    return range_pop_of_size_or_less(g_phys_memory_map.base, g_phys_memory_map.length,
                                     want_size, out_result, out_result_size);
}

int prot_to_mmu_flags(mmap_Protection prot)
{
    int flags = 0;

    if ((prot & MMAP_PROT_EXEC) == 0)  flags |= MMU_EXECUTE_DISABLE;
    if ((prot & MMAP_PROT_WRITE) != 0) flags |= MMU_READ_WRITE;
    if ((prot & MMAP_PROT_RING_3) != 0) flags |= MMU_USER_PAGE;

    return flags;
}

res mmap(void *addr, size_t size, mmap_Protection prot)
{
    assert(g_phys_memory_map.base);
    assert(((uint64_t)addr & 0xfff) == 0 && "addr must be page aligned");

    if ((prot & MMAP_PROT_READ) == 0) return res_mmap_MUST_BE_READABLE;
    const int mmu_flags = prot_to_mmu_flags(prot);

    if (size == 0) return res_mmap_INVALID_SIZE;

    uint64_t size_aligned = PAGE_ALIGN_UP(size);
    uint64_t addr_it = (uint64_t)addr;

    while (size_aligned)
    {
        uint64_t allocated_phys_addr;
        uint64_t allocated_size;
        bool success = allocate_physical_memory(size_aligned,
                                                &allocated_phys_addr, &allocated_size);
        // TODO: on fail, deallocate all the allocated memory
        if (!success) return res_mmap_OUT_OF_MEMORY;

        uint64_t begin = allocated_phys_addr;
        uint64_t end   = begin + allocated_size;
        mmu_map_range(begin, end, addr_it, mmu_flags);

        size_aligned -= allocated_size;
        addr_it += allocated_size;
    }

    return res_OK;
}

void munmap(void *in_addr, size_t size)
{
    uint8_t *addr = in_addr;

    assert(((uint64_t)addr & 0xfff) == 0 && "addr must be page aligned");

    range_Range cur = {0};

    for (uint8_t *it = addr; it < addr + size; it += PAGE_SIZE)
    {
        mmu_PageTableEntry *page = mmu_page_existing(it);
        uint64_t physical_page_addr = mmu_page_table_entry_address_get(page);

        page->present = 0;
        mmu_tlb_flush(it);

        bool is_right_after_current_range = cur.begin + cur.size == physical_page_addr;
        if (is_right_after_current_range)
        {
            cur.size += PAGE_SIZE;
            continue;
        }

        mmap_phys_memory_add(&cur);

        cur.begin = physical_page_addr;
        cur.size = PAGE_SIZE;
    }

    if (cur.size != 0)
    {
        mmap_phys_memory_add(&cur);
    }
}

res mprotect(void *addr, size_t size, mmap_Protection prot)
{
    assert(((uint64_t)addr & 0xfff) == 0 && "addr must be page aligned");

    if ((prot & MMAP_PROT_READ) == 0) return res_mmap_MUST_BE_READABLE;
    const int mmu_flags = prot_to_mmu_flags(prot);

    mmu_page_range_set_flags(addr, addr + size, mmu_flags);

    return res_OK;
}
