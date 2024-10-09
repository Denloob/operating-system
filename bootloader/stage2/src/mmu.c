#include "mmu.h"
#include "bitmap.h"
#include "bitmap.h"
#include "memory.h"
#include "assert.h"
#include <stdint.h>


#define TABLE_LENGTH 512
#define KILOBYTE 4096

#define TABLE_SIZE_BYTES (TABLE_LENGTH * sizeof(mmu_PageMapEntry))

#define MMU_TABLE_COUNT 512
#define MMU_BITMAP_SIZE (MMU_TABLE_COUNT / 8)
#define MMU_BITMAP_LENGTH MMU_TABLE_COUNT

#define MMU_MAP_BASE 0x100000

#define MMU_ALL_TABLES_SIZE (MMU_TABLE_COUNT * TABLE_SIZE_BYTES)
#define MMU_TOTAL_CHUNK_SIZE (MMU_BITMAP_SIZE + MMU_ALL_TABLES_SIZE)

#define MMU_BITMAP_BASE (MMU_MAP_BASE + MMU_ALL_TABLES_SIZE)

#define MMU_STRUCTURES_START MMU_MAP_BASE
#define MMU_STRUCTURES_END (MMU_STRUCTURES_START + MMU_TOTAL_CHUNK_SIZE)
#define BOOTLOADER_STAGE2_BEGIN 0
#define BOOTLOADER_STAGE2_END (2 * KILOBYTE)

#define STACK_END 0x8000
#define STACK_BEGIN (STACK_END - KILOBYTE * 3)

#define KERNEL_BEGIN 0x7000
#define KERNEL_END (KILOBYTE * 2)


Bitmap *mmu_tables_bitmap = (Bitmap *)MMU_BITMAP_BASE;

void *mmu_map_allocate()
{
    for (int i = 0; i < MMU_BITMAP_LENGTH; i++)
    {
        if (!bitmap_test(mmu_tables_bitmap, i))
        {
            bitmap_set(mmu_tables_bitmap, i);
            return (void *)(MMU_MAP_BASE + i * TABLE_SIZE_BYTES);
        }
    }

    // No block was found
    return 0;
}

void mmu_map_deallocate(void *address)
{
    int index = ((size_t)address - MMU_BITMAP_BASE) / TABLE_SIZE_BYTES;
    bitmap_clear(mmu_tables_bitmap, index);
}

mmu_PageMapEntry *g_pml4 = NULL;

mmu_PageTableEntry *mmu_page_allocate(uint64_t virtual, uint64_t physical)
{
    mmu_PageTableEntry *page = mmu_page((void *)virtual);
    mmu_page_table_entry_address_set(page, (uint64_t)physical);
    page->present = true;

    return page;
}

void mmu_init()
{
    bitmap_init(mmu_tables_bitmap, MMU_BITMAP_SIZE);
    g_pml4 = mmu_map_allocate();
    mmu_table_init(g_pml4);

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

    for (uint64_t it = STACK_BEGIN; it < STACK_END; it += KILOBYTE)
    {
        mmu_PageTableEntry *page = mmu_page_allocate(it, it);
        page->read_write = true;
    }

    for (uint64_t it = MMU_STRUCTURES_START; it < MMU_STRUCTURES_END; it += KILOBYTE)
    {
        mmu_PageTableEntry *page = mmu_page_allocate(it, it);
        page->read_write = true;
    }



    __asm__ volatile("mov eax, cr4\n"
                     "or eax, 1 << 5\n"
                     "mov cr4, eax\n"
                     :
                     :
                     : "eax", "cc");

    __asm__ volatile("mov cr3, %0\n" : : "a"(g_pml4));

#define EFER_MSR 0xC0000080
    __asm__ volatile("rdmsr\n"
                     "or eax, 1 << 8\n"
                     "wrmsr"
                     :
                     : "c"(EFER_MSR)
                     : "eax", "cc");

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
    return (mmu_PageMapEntry *)mmu_page_table_entry_address_get(entry);
}

mmu_PageMapEntry *mmu_page_map_get_or_allocate_of(mmu_PageMapEntry *entry)
{
    if (!entry->present)
    {
        uint64_t address = (uint64_t)mmu_map_allocate();
        assert(address && "mmu_map_allocate(): NULL");
        mmu_page_table_entry_address_set(entry, address);
        mmu_table_init((void *)mmu_page_table_entry_address_get(entry));
        entry->present = true;
    }

    return (mmu_PageMapEntry *)mmu_page_table_entry_address_get(entry);
}


mmu_PageTableEntry *mmu_page(void *address)
{
    uint64_t address64 = (uint64_t)address;
    mmu_PageIndexes page_indexes = *(mmu_PageIndexes *)&address64;


    // We set read_write to `true` for all layers except last one,
    //  as they are hierarchical, so even one false would mean that no page under can write.
    mmu_PageMapEntry *pml4_entry = g_pml4 + page_indexes.level4;
    pml4_entry->read_write = true;

    mmu_PageMapEntry *pml3 = mmu_page_map_get_or_allocate_of(pml4_entry);

    mmu_PageMapEntry *pml3_entry = pml3 + page_indexes.level3;
    pml3_entry->read_write = true;
    mmu_PageMapEntry *pml2 = mmu_page_map_get_or_allocate_of(pml3_entry);

    mmu_PageMapEntry *pml2_entry = pml2 + page_indexes.level2;
    pml2_entry->read_write = true;
    mmu_PageTableEntry *page_table = (mmu_PageTableEntry *)mmu_page_map_get_or_allocate_of(pml2_entry);

    mmu_PageTableEntry *entry = page_table + page_indexes.page;
    return entry;
}

#define MMU_ENTRY_ADDRESS_BITSHIFT 12 // So we aren't actually storing the address at entry->_address. Instead, we are storing `entry | address`. Confusing I know. Anyway, this means that the _address is actually bit shifted by 12.

uint64_t mmu_page_table_entry_address_get(void *page_map_ptr)
{
    return ((mmu_PageTableEntry *)page_map_ptr)->_address << MMU_ENTRY_ADDRESS_BITSHIFT;
}

void mmu_page_table_entry_address_set(void *page_map_ptr, uint64_t address)
{
    ((mmu_PageTableEntry *)page_map_ptr)->_address = address >> MMU_ENTRY_ADDRESS_BITSHIFT;
}
