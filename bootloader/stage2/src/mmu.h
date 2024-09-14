#pragma once

#include <stddef.h>
#include <stdint.h>

typedef struct
{
    uint64_t offset : 9;
    uint64_t page : 9;
    uint64_t level2 : 9;
    uint64_t level3 : 9;
    uint64_t level4 : 9;
} mmu_PageIndexes;

typedef struct
{
    uint64_t present : 1;
    uint64_t read_write : 1;
    uint64_t user_supervisor : 1;
    uint64_t write_through : 1;
    uint64_t cache_disable : 1;
    uint64_t accessed : 1;
    uint64_t dirty : 1;
    uint64_t pat : 1;
    uint64_t global : 1;
    uint64_t
        available3 : 3; // unused by the MMU, can be used by the kernel (same for any of the fields called available)
    uint64_t address : 40;
    uint64_t available7 : 7;
    uint64_t protection_key : 4;
    uint64_t execute_disable : 1;
} mmu_PageTableEntry;

typedef struct
{
    uint64_t present : 1;
    uint64_t read_write : 1;
    uint64_t user_supervisor : 1;
    uint64_t write_through : 1;
    uint64_t cache_disable : 1;
    uint64_t accessed : 1;
    uint64_t available1 : 1;
    uint64_t page_size : 1; // set to 0
    uint64_t available4 : 4;
    uint64_t address : 40;
    uint64_t available11 : 11;
    uint64_t execute_disable : 1;
} mmu_PageMapEntry;

extern mmu_PageMapEntry *pml4;

void mmu_page_map_init(mmu_PageMapEntry *address);
void mmu_tlb_flush(size_t virtual_address);
mmu_PageMapEntry *mmu_page_map_get_address_from(mmu_PageMapEntry *page_map);
mmu_PageTableEntry *mmu_page(void *address);
