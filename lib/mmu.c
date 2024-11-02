#include "mmu.h"
#include "range.h"
#include "bitmap.h"
#include "bitmap.h"
#include "memory.h"
#include "assert.h"
#include <stdint.h>


#define TABLE_LENGTH 512
#define KILOBYTE 4096
#define PAGE_SIZE KILOBYTE
#define PAGE_ALIGN_UP(address)   (((address) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))
#define PAGE_ALIGN_DOWN(address) ((address) & ~(PAGE_SIZE - 1))

#define TABLE_SIZE_BYTES (TABLE_LENGTH * sizeof(mmu_PageMapEntry))

#define MMU_TABLE_COUNT 512
#define MMU_BITMAP_SIZE (MMU_TABLE_COUNT / 8)
#define MMU_BITMAP_LENGTH MMU_TABLE_COUNT

uint64_t g_mmu_map_base_address = 0; // NOTE: this has to be not in the bss, as we cannot map the bss pages without this variable, and cannot set this variable (if it were in the bss) without mapping the bss pages.
Bitmap *mmu_tables_bitmap = 0;
mmu_PageMapEntry *g_pml4 = 0;

#define MMU_ALL_TABLES_SIZE (MMU_TABLE_COUNT * TABLE_SIZE_BYTES)
#define MMU_TOTAL_CHUNK_SIZE (MMU_BITMAP_SIZE + MMU_ALL_TABLES_SIZE)

#define MMU_BITMAP_BASE (g_mmu_map_base_address + MMU_ALL_TABLES_SIZE)

#define MMU_STRUCTURES_START g_mmu_map_base_address
#define MMU_STRUCTURES_END (MMU_STRUCTURES_START + MMU_TOTAL_CHUNK_SIZE)
#define BOOTLOADER_STAGE2_BEGIN 0

#define STACK_END 0x8000
#define STACK_BEGIN (STACK_END - KILOBYTE * 3)

#define VGA_BEGIN 0xb8000
#define VGA_END 0xb8fa0

void *mmu_map_allocate()
{
    for (int i = 0; i < MMU_BITMAP_LENGTH; i++)
    {
        if (!bitmap_test(mmu_tables_bitmap, i))
        {
            bitmap_set(mmu_tables_bitmap, i);
            return (void *)(g_mmu_map_base_address + i * TABLE_SIZE_BYTES);
        }
    }

    // No block was found
    return 0;
}

void mmu_map_deallocate(void *address)
{
    int index = ((size_t)address - MMU_STRUCTURES_START) / TABLE_SIZE_BYTES;
    bitmap_clear(mmu_tables_bitmap, index);
}

mmu_PageTableEntry *mmu_page_allocate(uint64_t virtual, uint64_t physical)
{
    mmu_PageTableEntry *page = mmu_page((void *)virtual);
    mmu_page_table_entry_address_set(page, (uint64_t)physical);
    page->present = true;

    return page;
}

/**
 * @brief - This function initializes global variables the mmu code depends on.
 *          It shall be called ONLY after mmu_init has ran. It's purpose is to run
 *          in the same context but different linkage of the mmu compilation unit.
 *          For example: After MMU is initialized by the bootloader, call
 *              this from the kernel.
 *
 * @param mmu_map_base_address The base address the mmu data is located at.
 */
void mmu_init_post_init(uint64_t mmu_map_base_address)
{
    g_mmu_map_base_address = mmu_map_base_address;
    mmu_tables_bitmap = (Bitmap *)MMU_BITMAP_BASE; // This macro depends on g_mmu_map_base_address
    g_pml4 = (mmu_PageMapEntry *)g_mmu_map_base_address; // As asserted bellow, this always will be the case.
}

uint64_t mmu_init(range_Range *memory_map, uint64_t memory_map_length, uint64_t bootloader_end_addr)
{
    for (int i = 0; i < memory_map_length; i++)
    {
        if (memory_map[i].size >= MMU_TOTAL_CHUNK_SIZE)
        {
            g_mmu_map_base_address = memory_map[i].begin;
            memory_map[i].begin += PAGE_ALIGN_UP(MMU_TOTAL_CHUNK_SIZE);
            memory_map[i].size -= PAGE_ALIGN_UP(MMU_TOTAL_CHUNK_SIZE);
            break;
        }
    }
    assert(g_mmu_map_base_address && "No consecutive physical RAM for the MMU structures were found\n");
    mmu_tables_bitmap = (Bitmap *)MMU_BITMAP_BASE;

    bitmap_init(mmu_tables_bitmap, MMU_BITMAP_SIZE);
    g_pml4 = mmu_map_allocate();
    assert((g_pml4 == (void *)g_mmu_map_base_address) && "Expected g_pml4 to be located at the first available address");
    mmu_table_init(g_pml4);

    mmu_map_range(BOOTLOADER_STAGE2_BEGIN, bootloader_end_addr, BOOTLOADER_STAGE2_BEGIN, MMU_READ_WRITE);
    mmu_map_range(STACK_BEGIN, STACK_END, STACK_BEGIN, MMU_READ_WRITE);
    mmu_map_range(MMU_STRUCTURES_START, MMU_STRUCTURES_END, MMU_STRUCTURES_START, MMU_READ_WRITE);
    mmu_map_range(VGA_BEGIN, VGA_END, VGA_BEGIN, MMU_READ_WRITE);

#ifdef __x86_64__
#define EAX_RAX "rax"
#else
#define EAX_RAX "eax"
#endif
    __asm__ volatile("mov " EAX_RAX ", cr4\n"
                     "or eax, 1 << 5\n"
                     "mov cr4, " EAX_RAX "\n"
                     :
                     :
                     : EAX_RAX, "cc");

    __asm__ volatile("mov cr3, %0\n" : : "a"(g_pml4));

#define EFER_MSR 0xC0000080 // This is the MSR (model-specific register) address for the EFER register
    __asm__ volatile("rdmsr\n"
                     "or eax, (1 << 8) | (1 << 11) \n" // Set the long mode bit and no-execute enable (allows MMU_EXECUTE_DISABLE flag to work in paging)
                     "wrmsr"
                     :
                     : "c"(EFER_MSR)
                     : "eax", "cc");

    __asm__ volatile("mov " EAX_RAX ", cr0\n"
                     "or eax, (1 << 31) | (1 << 16) \n" // Set the paging bit, the write-protect bit (kernel too is subject to paging limiting the write ability)
                     "mov cr0, " EAX_RAX "\n"
                     :
                     :
                     : EAX_RAX, "cc");

    return g_mmu_map_base_address;
}

void mmu_table_init(void *address)
{
    memset(address, 0, TABLE_SIZE_BYTES);
}

void mmu_tlb_flush(size_t virtual_address)
{
    __asm__ volatile("invlpg (%0)" : : "m"(virtual_address) : "memory");
}

void mmu_tlb_flush_all()
{
    unsigned long cr3; // We don't actually need the value of cr3, but by using the `long` type, gcc will automatically compile it into eax on IA-32 and rax on amd64
    asm volatile ("mov %0, cr3\n"
                  "mov cr3, %0\n" : "=a"(cr3) : : "memory");
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

mmu_PageTableEntry *mmu_page_existing(void *address)
{
    uint64_t address64 = (uint64_t)address;
    mmu_PageIndexes page_indexes = *(mmu_PageIndexes *)&address64;

    mmu_PageMapEntry *pml4_entry = g_pml4 + page_indexes.level4;
    mmu_PageMapEntry *pml3 = mmu_page_map_get_address_of(pml4_entry);
    mmu_PageMapEntry *pml3_entry = pml3 + page_indexes.level3;
    mmu_PageMapEntry *pml2 = mmu_page_map_get_address_of(pml3_entry);
    mmu_PageMapEntry *pml2_entry = pml2 + page_indexes.level2;
    mmu_PageTableEntry *page_table = (mmu_PageTableEntry *)mmu_page_map_get_address_of(pml2_entry);

    mmu_PageTableEntry *entry = page_table + page_indexes.page;
    assert(entry->present && "The page must be valid");
    return entry;
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

void mmu_map_range(uint64_t physical_begin, uint64_t physical_end,
                   uint64_t virtual_begin, int flags)
{
    for (uint64_t virt = virtual_begin & (~0xfff), phys = physical_begin & (~0xfff);
         phys < physical_end; phys += KILOBYTE, virt += KILOBYTE)
    {
        mmu_PageTableEntry *page = mmu_page_allocate(virt, phys);
        page->read_write = flags & MMU_READ_WRITE;
        page->execute_disable = flags & MMU_EXECUTE_DISABLE;
    }
}

void mmu_page_set_flags(void *virtual_address, int new_flags)
{
    mmu_PageTableEntry *page = mmu_page_existing(virtual_address);

    page->read_write = new_flags & MMU_READ_WRITE;
    page->execute_disable = new_flags & MMU_EXECUTE_DISABLE;
}

void mmu_page_range_set_flags(void *virtual_address_begin, void *virtual_address_end, int new_flags)
{
    uint64_t virtual_begin = (uint64_t)virtual_address_begin & (~0xfff);
    uint64_t virtual_end = (uint64_t)virtual_address_end;

    for (uint64_t virt = virtual_begin; virt < virtual_end; virt += KILOBYTE)
    {
        mmu_page_set_flags(virtual_address_end, new_flags);
    }
}
