#include "mmu.h"
#include "memory.h"
#include "assert.h"

#define TABLE_SIZE 512

mmu_PageMapEntry *pml4;

void mmu_page_map_init(mmu_PageMapEntry *address)
{
    memset(address, 0, TABLE_SIZE * sizeof(mmu_PageMapEntry));
}

void mmu_tlb_flush(size_t virtual_address)
{
    __asm__ volatile("invlpg (%0)" : : "m"(virtual_address) : "memory");
}

mmu_PageMapEntry *mmu_page_map_get_address_from(mmu_PageMapEntry *page_map)
{
    assert(page_map->present && "The page map must be valid");
    return (mmu_PageMapEntry *)(page_map->address);
}

mmu_PageTableEntry *mmu_page(void *address)
{
    mmu_PageIndexes page_indexes = *(mmu_PageIndexes *)&address;

    mmu_PageMapEntry *pml3 = mmu_page_map_get_address_from(pml4 + page_indexes.level4);
    mmu_PageMapEntry *pml2 = mmu_page_map_get_address_from(pml3 + page_indexes.level3);
    mmu_PageTableEntry *page_table = (mmu_PageTableEntry *)mmu_page_map_get_address_from(pml2 + page_indexes.level2);
    return page_table + page_indexes.page;
}
