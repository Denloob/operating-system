#include "mmu.h"
#include "range.h"
#include "bitmap.h"
#include "bitmap.h"
#include "memory.h"
#include "assert.h"
#include "math.h"
#include "mmu_config.h"
#include <stdint.h>

#define PAGE_SIZE 0x1000
#define PAGE_ALIGN_UP(address)   math_ALIGN_UP(address, PAGE_SIZE)
#define PAGE_ALIGN_DOWN(address) math_ALIGN_DOWN(address, PAGE_SIZE)

uint64_t g_mmu_map_base_address = 0; // NOTE: this has to be not in the bss, as we cannot map the bss pages without this variable, and cannot set this variable (if it were in the bss) without mapping the bss pages.
uint64_t g_mmu_phys_delta = 0;
Bitmap *mmu_tables_bitmap = 0;
mmu_PageMapEntry *g_pml4 = 0;

#define MMU_BITMAP_BASE (g_mmu_map_base_address + MMU_ALL_TABLES_SIZE)

#define MMU_STRUCTURES_START g_mmu_map_base_address
#define MMU_STRUCTURES_END (MMU_STRUCTURES_START + MMU_TOTAL_CHUNK_SIZE)

#define STACK_END 0x10000
#define STACK_BEGIN (STACK_END - PAGE_SIZE * 3)

#define BOOTLOADER_STAGE2_BEGIN 0
#define BOOTLOADER_STAGE2_END (STACK_BEGIN - PAGE_SIZE) // HACK: the bootloader "ends" one page before its stack, thus allowing us to easily unmap it. This value is validated in mmu_init

#define VGA_BEGIN 0xb8000
#define VGA_END 0xb8fa0

void mmu_unmap_range(uint64_t virtual_begin, uint64_t virtual_end);

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
    mmu_PageTableEntry *page = mmu_page(virtual);
    mmu_page_table_entry_address_set(page, (uint64_t)physical);
    page->present = true;

    return page;
}

void mmu_unmap_bootloader()
{
    mmu_unmap_range(BOOTLOADER_STAGE2_BEGIN, BOOTLOADER_STAGE2_END);
    mmu_unmap_range(STACK_BEGIN, STACK_END);
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

void mmu_migrate_to_virt_mem(void *addr) // addr is of size MMU_TOTAL_CHUNK_SIZE
{
    mmu_map_range(g_mmu_map_base_address, g_mmu_map_base_address + MMU_TOTAL_CHUNK_SIZE, (uint64_t)addr, MMU_READ_WRITE);

    uint64_t old_phys = g_mmu_map_base_address;
    g_mmu_phys_delta = (uint64_t)addr - g_mmu_map_base_address;
    g_mmu_map_base_address = (uint64_t)addr;
    mmu_tables_bitmap = (Bitmap *)MMU_BITMAP_BASE; // This macro depends on g_mmu_map_base_address
    g_pml4 = (mmu_PageMapEntry *)g_mmu_map_base_address; // As asserted bellow, this always will be the case.

    mmu_unmap_range(old_phys, old_phys + MMU_TOTAL_CHUNK_SIZE);
}

uint64_t mmu_init(range_Range *memory_map, uint64_t memory_map_length, uint64_t bootloader_end_addr)
{
    bool success = range_pop_of_size(memory_map, memory_map_length, PAGE_ALIGN_UP(MMU_TOTAL_CHUNK_SIZE), &g_mmu_map_base_address);
    assert(success && "No consecutive physical RAM for the MMU structures were found\n");

    mmu_tables_bitmap = (Bitmap *)MMU_BITMAP_BASE;

    bitmap_init(mmu_tables_bitmap, MMU_BITMAP_SIZE);
    g_pml4 = mmu_map_allocate();
    assert((g_pml4 == (void *)g_mmu_map_base_address) && "Expected g_pml4 to be located at the first available address");
    mmu_table_init(g_pml4);

    mmu_map_range(BOOTLOADER_STAGE2_BEGIN, BOOTLOADER_STAGE2_END, BOOTLOADER_STAGE2_BEGIN, MMU_READ_WRITE);
    assert(bootloader_end_addr <= BOOTLOADER_STAGE2_END && "Oops, we ran out of the reserved bootloader space.");
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
                     : "eax", "edx", "cc");

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

void mmu_tlb_flush(void *virtual_address)
{
    __asm__ volatile("invlpg [%0]" : : "r"(virtual_address) : "memory");
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
    return (mmu_PageMapEntry *)mmu_page_table_entry_address_get_virt(entry);
}

mmu_PageMapEntry *mmu_page_map_get_or_allocate_of(mmu_PageMapEntry *entry)
{
    if (!entry->present)
    {
        uint64_t address = (uint64_t)mmu_map_allocate();
        assert(address && "mmu_map_allocate(): NULL");
        mmu_page_table_entry_address_set_virt(entry, address);
        mmu_table_init((void *)mmu_page_table_entry_address_get_virt(entry));
        entry->present = true;
    }

    return (mmu_PageMapEntry *)mmu_page_table_entry_address_get_virt(entry);
}

mmu_PageTableEntry *mmu_page_existing(void *address)
{
    const bool is_valid_virtual_address = ((uint64_t)address < 0x0000800000000000 || (uint64_t)address > 0xFFFF7FFFFFFFFFFF);
    assert(is_valid_virtual_address && "Highest 16 bits of an address must be the same (aka either canonical address, or non-canonical)");

    mmu_PageIndexes page_indexes = {0};
    memmove(&page_indexes, &address, sizeof(address));

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

mmu_PageTableEntry *mmu_page(uint64_t address)
{
    const bool is_valid_virtual_address = (address < 0x0000800000000000 || address > 0xFFFF7FFFFFFFFFFF);
    assert(is_valid_virtual_address && "Highest 16 bits of an address must be the same (aka either canonical address, or non-canonical)");

    mmu_PageIndexes page_indexes = {0};
    memmove(&page_indexes, &address, sizeof(address));


    // We set read_write to `true` for all layers except last one,
    //  as they are hierarchical, so even one false would mean that no page under can write.
    mmu_PageMapEntry *pml4_entry = g_pml4 + page_indexes.level4;
    pml4_entry->read_write = true;
    pml4_entry->user_supervisor = true;

    mmu_PageMapEntry *pml3 = mmu_page_map_get_or_allocate_of(pml4_entry);

    mmu_PageMapEntry *pml3_entry = pml3 + page_indexes.level3;
    pml3_entry->read_write = true;
    pml3_entry->user_supervisor = true;
    mmu_PageMapEntry *pml2 = mmu_page_map_get_or_allocate_of(pml3_entry);

    mmu_PageMapEntry *pml2_entry = pml2 + page_indexes.level2;
    pml2_entry->read_write = true;
    pml2_entry->user_supervisor = true;
    mmu_PageTableEntry *page_table = (mmu_PageTableEntry *)mmu_page_map_get_or_allocate_of(pml2_entry);

    mmu_PageTableEntry *entry = page_table + page_indexes.page;
    return entry;
}

#define MMU_ENTRY_ADDRESS_BITSHIFT 12 // So we aren't actually storing the address at entry->_address. Instead, we are storing `entry | address`. Confusing I know. Anyway, this means that the _address is actually bit shifted by 12.

uint64_t mmu_page_table_entry_address_get(mmu_PageTableEntry *page_map_ptr)
{
    uint64_t phys_address = (uint64_t)(page_map_ptr->_address) << MMU_ENTRY_ADDRESS_BITSHIFT;

    return phys_address;
}

void mmu_page_table_entry_address_set(mmu_PageTableEntry *page_map_ptr, uint64_t address)
{
    page_map_ptr->_address = address >> MMU_ENTRY_ADDRESS_BITSHIFT;
}

uint64_t mmu_page_table_entry_address_get_virt(mmu_PageMapEntry *page_map_ptr)
{
    uint64_t phys_address = (uint64_t)(((mmu_PageTableEntry *)page_map_ptr)->_address) << MMU_ENTRY_ADDRESS_BITSHIFT;

    return phys_address + g_mmu_phys_delta;
}

void mmu_page_table_entry_address_set_virt(mmu_PageMapEntry *page_map_ptr, uint64_t address)
{
    uint64_t phys_address = address - g_mmu_phys_delta;
    ((mmu_PageTableEntry *)page_map_ptr)->_address = phys_address >> MMU_ENTRY_ADDRESS_BITSHIFT;
}

void mmu_unmap_range(uint64_t virtual_begin, uint64_t virtual_end)
{
    for (uint64_t virt = virtual_begin & (~0xfff);
         virt < virtual_end; virt += PAGE_SIZE)
    {
        mmu_PageTableEntry *page = mmu_page_existing((void *)virt);
        page->present = 0;
    }
}

void mmu_map_range(uint64_t physical_begin, uint64_t physical_end,
                   uint64_t virtual_begin, int flags)
{
    for (uint64_t virt = virtual_begin & (~0xfff), phys = physical_begin & (~0xfff);
         phys < physical_end; phys += PAGE_SIZE, virt += PAGE_SIZE)
    {
        mmu_PageTableEntry *page = mmu_page_allocate(virt, phys);
        page->read_write = (flags & MMU_READ_WRITE) != 0;
        page->execute_disable = (flags & MMU_EXECUTE_DISABLE) != 0;
        page->user_supervisor = (flags & MMU_USER_PAGE) != 0;
    }
}

void mmu_page_set_flags(void *virtual_address, int new_flags)
{
    mmu_PageTableEntry *page = mmu_page_existing(virtual_address);

    page->read_write = (new_flags & MMU_READ_WRITE) != 0;
    page->execute_disable = (new_flags & MMU_EXECUTE_DISABLE) != 0;
    page->user_supervisor = (new_flags & MMU_USER_PAGE) != 0;
}

void mmu_page_range_set_flags(void *virtual_address_begin, void *virtual_address_end, int new_flags)
{
    uint64_t virtual_begin = (uint64_t)virtual_address_begin & (~0xfff);
    uint64_t virtual_end = (uint64_t)virtual_address_end;

    for (uint64_t virt = virtual_begin; virt < virtual_end; virt += PAGE_SIZE)
    {
        mmu_page_set_flags((void *)virt, new_flags);
    }
}

uint64_t mmu_get_phys_addr_of(void *virt)
{
    mmu_PageTableEntry *page = mmu_page_existing(virt);
    return mmu_page_table_entry_address_get(page);
}

void mmu_load_virt_pml4(void *pml4)
{
    g_pml4 = pml4;
    asm volatile("mov cr3, %0" :: "r"((size_t)mmu_get_phys_addr_of(pml4)) : "memory");
}
