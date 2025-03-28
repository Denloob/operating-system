#pragma once

#include "range.h"
#include <stddef.h>
#include <stdint.h>

enum {
    MMU_READ_WRITE      = 0x1,
    MMU_EXECUTE_DISABLE = 0x2,
    MMU_USER_PAGE       = 0x4,
};

typedef struct
{
    uint64_t offset : 12;
    uint64_t page : 9;
    uint64_t level2 : 9;
    uint64_t level3 : 9;
    uint64_t level4 : 9;
    uint64_t pad : 16;
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
    uint64_t _address : 40;
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
    uint64_t _address : 40; // DO NOT ACCESS DIRECTLY! use mmu_page_table_entry_address_set or _get
    uint64_t available11 : 11;
    uint64_t execute_disable : 1;
} mmu_PageMapEntry;

extern mmu_PageMapEntry *g_pml4;

void mmu_init_post_init(uint64_t mmu_map_base_address);
uint64_t mmu_init(range_Range *memory_map, uint64_t memory_map_length, uint64_t bootloader_end_addr);
uint64_t mmu_page_table_entry_address_get(mmu_PageTableEntry *page_map_ptr);
void mmu_page_table_entry_address_set(mmu_PageTableEntry *page_map_ptr, uint64_t address);
uint64_t mmu_page_table_entry_address_get_virt(mmu_PageMapEntry *page_map_ptr);
void mmu_page_table_entry_address_set_virt(mmu_PageMapEntry *page_map_ptr, uint64_t address);
mmu_PageTableEntry *mmu_page_existing(void *address);
void mmu_page_set_flags(void *virtual_address, int new_flags);
void mmu_page_range_set_flags(void *virtual_address_begin, void *virtual_address_end, int new_flags);
void mmu_table_init(void *address);
void mmu_tlb_flush(void *virtual_address);
void mmu_tlb_flush_all();
mmu_PageMapEntry *mmu_page_map_get_address_of(mmu_PageMapEntry *entry);
mmu_PageTableEntry *mmu_page(uint64_t address);

void mmu_migrate_to_virt_mem(void *addr);
void mmu_load_virt_pml4(void *pml4);

uint64_t mmu_get_phys_addr_of(void *virt);

/**
 * @brief Allocate a page for an MMU structure (like pml4).
 *
 * @return 4096 byte aligned address of size 4096.
 */
void *mmu_map_allocate();

/**
 * @brief Dealloc the result of mmu_map_allocate.
 *
 * @see mmu_map_allocate
 *
 * @param address The address returned by mmu_map_allocate.
 */
void mmu_map_deallocate(void *address);

void mmu_map_range(uint64_t physical_begin, uint64_t physical_end, uint64_t virtual_begin, int flags);
void mmu_unmap_range(uint64_t virtual_begin, uint64_t virtual_end);

/**
 * @brief Unmap the bootloader and the bootloader stack. Does not return the physical memory (the bootloader should do that). Note that the bootloader contains the GDT which should be set before unmapping it.
 */
void mmu_unmap_bootloader();
