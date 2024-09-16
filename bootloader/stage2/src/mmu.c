#include "mmu.h"
#include "memory.h"
#include "assert.h"

#define BOOTLOADER_STAGE2_BASE_ADDR 0x500

#define TABLE_SIZE 512
#define TABLE_SIZE_BYTES (TABLE_SIZE * sizeof(mmu_PageMapEntry))
#define RAM_ADDRESS_START 0x00100000
#define KILOBYTE 4096

mmu_PageMapEntry *pml4 = (mmu_PageMapEntry *)RAM_ADDRESS_START;
void mmu_page_map_entry_add(mmu_PageMapEntry *table, int index, uint64_t address)
{
    table[index].address = address;
    table[index].present = true;
}

void mmu_init()
{
    mmu_PageMapEntry *pml3 = (mmu_PageMapEntry *)(RAM_ADDRESS_START+TABLE_SIZE_BYTES);
    mmu_PageMapEntry *pml2 = (mmu_PageMapEntry *)(RAM_ADDRESS_START+TABLE_SIZE_BYTES*2);
    mmu_PageTableEntry *page_table = (mmu_PageTableEntry *)(RAM_ADDRESS_START+TABLE_SIZE_BYTES*3);

    mmu_table_init(pml4);
    mmu_page_map_entry_add(pml4, 0, (uint64_t)pml3);
    mmu_table_init(pml3);
    mmu_page_map_entry_add(pml3, 0, (uint64_t)pml2);
    mmu_table_init(pml2);
    mmu_page_map_entry_add(pml2, 0, (uint64_t)page_table);
    mmu_table_init(page_table);
    for (uint64_t it = (uint64_t)pml4; it < (uint64_t)page_table + TABLE_SIZE_BYTES; it += KILOBYTE)
    {
        mmu_PageTableEntry *page = mmu_page((void *)it);
        page->read_write = true;
        page->present = true;
        page->address = it;
    }

    for (uint64_t it = BOOTLOADER_STAGE2_BASE_ADDR; it < (uint64_t)BOOTLOADER_STAGE2_BASE_ADDR + 512 * 20; it += KILOBYTE)
    {
        mmu_PageTableEntry *page = mmu_page((void *)it);
        page->present = true;
        page->address = it;
    }

    __asm__ volatile("mov cr3, %0\n" : : "a"(pml4));
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

mmu_PageMapEntry *mmu_page_map_get_address_from(mmu_PageMapEntry *page_map)
{
    assert(page_map->present && "The page map must be valid");
    return (mmu_PageMapEntry *)(page_map->address);
}

mmu_PageTableEntry *mmu_page(void *address)
{
    mmu_PageIndexes page_indexes = *(mmu_PageIndexes *)&address;

    // TODO: check if pages present
    mmu_PageMapEntry *pml3 = mmu_page_map_get_address_from(pml4 + page_indexes.level4);
    mmu_PageMapEntry *pml2 = mmu_page_map_get_address_from(pml3 + page_indexes.level3);
    mmu_PageTableEntry *page_table = (mmu_PageTableEntry *)mmu_page_map_get_address_from(pml2 + page_indexes.level2);
    return page_table + page_indexes.page;
}
