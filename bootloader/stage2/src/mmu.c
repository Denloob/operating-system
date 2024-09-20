#include "mmu.h"
#include "bitmap.h"
#include "bitmap.h"
#include "memory.h"
#include "assert.h"
#include <stdint.h>


#define TABLE_SIZE 512
#define KILOBYTE 4096

#define TABLE_SIZE_BYTES (TABLE_SIZE * sizeof(mmu_PageMapEntry))

#define MMU_TABLE_COUNT 512
#define MMU_BITMAP_SIZE (MMU_TABLE_COUNT / 8) 

#define MMU_BITMAP_BASE 0x100000
#define MMU_MAP_BASE (MMU_BITMAP_BASE + MMU_BITMAP_SIZE)

#define MMU_TOTAL_CHUNK_SIZE (MMU_BITMAP_SIZE + MMU_TABLE_COUNT*TABLE_SIZE)
#define MMU_STRUCTURES_START MMU_BITMAP_BASE
#define MMU_STRUCTURES_END (MMU_STRUCTURES_START + MMU_TOTAL_CHUNK_SIZE)
#define BOOTLOADER_STAGE2_BEGIN 0
#define BOOTLOADER_STAGE2_END (2 * KILOBYTE)
#define KERNEL_BEGIN 0x7000
#define KERNEL_END (KILOBYTE * 2)


void mmu_page_map_entry_add(mmu_PageMapEntry *table, int index, uint64_t address)
{
    table[index].address = address;
    table[index].present = true;
}

Bitmap *mmu_tables_bitmap = (Bitmap *)MMU_BITMAP_BASE;

void *mmu_map_allocate()
{
    for (int i = 0; i < MMU_BITMAP_SIZE; i++) 
    {
        if (!bitmap_test(mmu_tables_bitmap, i)) 
        { 
            bitmap_set(mmu_tables_bitmap, i);
            return (void *)(MMU_MAP_BASE + i * TABLE_SIZE);
        }
    }

    // No block was found
    return 0;
}

void mmu_map_deallocate(void *address)
{
    int index = ((size_t)address - MMU_BITMAP_BASE) / TABLE_SIZE;
    bitmap_clear(mmu_tables_bitmap, index);
}

mmu_PageMapEntry *g_pml4 = NULL;

mmu_PageTableEntry *mmu_page_allocate(uint64_t virtual, uint64_t physical)
{
    mmu_PageTableEntry *page = mmu_page((void *)virtual);
    page->address = (uint64_t)physical;
    page->present = true;

    return page;
}

void mmu_init()
{
    bitmap_init(mmu_tables_bitmap, MMU_BITMAP_SIZE);
    g_pml4 = mmu_map_allocate();
    mmu_table_init(g_pml4);

    for (uint64_t it = MMU_STRUCTURES_START; it < MMU_STRUCTURES_END; it += KILOBYTE)
    {
        mmu_PageTableEntry *page = mmu_page_allocate(it, it);
        page->read_write = true;
    }

    for (uint64_t it = BOOTLOADER_STAGE2_BEGIN; it < (uint64_t)BOOTLOADER_STAGE2_END; it += KILOBYTE)
    {
        mmu_PageTableEntry *page = mmu_page_allocate(it, it);
        page->read_write = true;
    }

    for (uint64_t it = KERNEL_BEGIN; it < (uint64_t)KERNEL_END; it += KILOBYTE)
    {
        mmu_PageTableEntry *page = mmu_page_allocate(it, it);
        page->read_write = true;
    }

#define EFER_MSR 0xC0000080
    __asm__ volatile("rdmsr\n"
                     "or eax, 1 << 8\n"
                     "wrmsr"
                     :
                     : "c"(EFER_MSR)
                     : "eax", "cc");

    __asm__ volatile("mov cr3, %0\n" : : "a"(g_pml4));
    __asm__ volatile("mov eax, cr0\n"
                     "or eax, 1 << 31\n"
                     "mov cr0, eax\n"
                     :
                     :
                     : "eax", "cc");
}

void mmu_table_init(void *address)
{
    memset(address, 0, TABLE_SIZE_BYTES);
}

void mmu_tlb_flush(size_t virtual_address)
{
    __asm__ volatile("invlpg (%0)" : : "m"(virtual_address) : "memory");
}

mmu_PageMapEntry *mmu_page_map_get_address_of(mmu_PageMapEntry *entry)
{
    assert(entry->present && "The page map must be valid");
    return (mmu_PageMapEntry *)(entry->address);
}

mmu_PageMapEntry *mmu_page_map_get_or_allocate_of(mmu_PageMapEntry *entry)
{
    if (!entry->present)
    {
        entry->address = (uint64_t)mmu_map_allocate();
        mmu_table_init((void *)entry->address);
        entry->present = true;
    }

    return (mmu_PageMapEntry *)entry->address;
}


mmu_PageTableEntry *mmu_page(void *address)
{
    mmu_PageIndexes page_indexes = *(mmu_PageIndexes *)&address;

    mmu_PageMapEntry *pml3 = mmu_page_map_get_or_allocate_of(g_pml4 + page_indexes.level4);
    mmu_PageMapEntry *pml2 = mmu_page_map_get_or_allocate_of(pml3 + page_indexes.level3);
    mmu_PageTableEntry *page_table = (mmu_PageTableEntry *)mmu_page_map_get_or_allocate_of(pml2 + page_indexes.level2);
    mmu_PageTableEntry *entry = page_table + page_indexes.page;

    return entry;
}
