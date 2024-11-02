#include "FAT16.h"
#include "range.h"
#include "assert.h"
#include "bios.h"
#include "drive.h"
#include "gdt.h"
#include "io.h"
#include "main.h"
#include "mmu.h"
#include "memory.h"

// The start and the end of the bootloader
extern char __entry_start;
extern char __end;

#define PAGE_SIZE 0x1000
#define PAGE_ALIGN_UP(address)   (((address) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))
#define PAGE_ALIGN_DOWN(address) ((address) & ~(PAGE_SIZE - 1))

typedef void (*kernel_main)(Drive drive);
#define KERNEL_BEGIN 0x7e00
#define KERNEL_BASE_ADDRESS (kernel_main)KERNEL_BEGIN

#define MEMORY_MAP_MAX_LENGTH (0x1000 / sizeof(range_Range)) // HACK: having all the ranges fit inside a single page makes implementation easier.
// Following number is chosen because the range 0x00007E00..0x0007FFFF should be free of use.
#define MEMORY_MAP_PHYSICAL_ADDR 0x8000

void get_memory_map(range_Range **resulting_memory_map, uint64_t *resulting_memory_map_size);

void start(uint16_t drive_id)
{
    uint64_t memory_map_length = 0;
    range_Range *memory_map = NULL;
    get_memory_map(&memory_map, &memory_map_length);
    assert(memory_map && memory_map_length && "Out of memory");

    printf("[*] Initializing drive #%d\n", (int)drive_id);
    Drive drive;
    if (!drive_init(&drive, drive_id))
    {
        assert(false && "drive_init failed");
    }

    printf("[*] Reading FAT16\n");
    fat16_Ref fat16;
    bool success = fat16_ref_init(&fat16, &drive);
    assert(success && "fat16_ref_init");

    printf("[*] Loading the kernel\n");
    fat16_File file;
    success = fat16_open(&fat16, "KERNEL  BIN", &file); //  HACK: fat16 should be case insensitive!
    assert(success && "fat16_open");

    uint8_t *kernel_physical_address = NULL;
    for (int i = 0; i < memory_map_length; i++)
    {
        if (memory_map[i].size >= file.file_entry.fileSize)
        {
            kernel_physical_address = (void *)memory_map[i].begin;
            memory_map[i].begin += PAGE_ALIGN_UP(file.file_entry.fileSize);
            memory_map[i].size -= PAGE_ALIGN_UP(file.file_entry.fileSize);
            break;
        }
    }
    assert(kernel_physical_address && "No consecutive physical RAM for the kernel was found\n");
    printf("[*] Chose kernel location - 0x%lx", kernel_physical_address);

    success = fat16_read(&file, kernel_physical_address);
    assert(success && "fat16_read");

    printf("[*] Initializing paging\n");
    mmu_init(memory_map, memory_map_length);

    printf("[*] Preparing for warp jump...\n");

    mmu_map_range((uint64_t)kernel_physical_address, KERNEL_BEGIN + file.file_entry.fileSize, KERNEL_BEGIN, MMU_READ_WRITE);

    main_gdt_long_mode_init();

    printf("\n[+] Coordinates locked, warp core stable\n");
    printf("Press any key to launch...\n");
    io_wait_key_raw();
    main_long_mode_jump_to(KERNEL_BASE_ADDRESS);
}

void main_gdt_long_mode_init()
{
    gdt_entry *gdt = &main_gdt_64bit;
    main_gdt_descriptor.offset = (uint64_t)gdt;

    // Code
    gdt[1] = (gdt_entry){
        .limit_low = 0xffff,
        .base_low = 0,
        .access = GDT_SEG_PRES | GDT_SEG_DESCTYPE_NOT_SYSTEM | GDT_SEG_CODE_EXRD | GDT_SEG_ACCESSED,
        .flags = GDT_SEG_GRAN | GDT_SEG_LONG,
        .limit_high = 0xf,
        .base_high = 0,
    };

    // Data
    gdt[2] = (gdt_entry){
        .limit_low = 0xffff,
        .base_low = 0,
        .access = GDT_SEG_PRES | GDT_SEG_DESCTYPE_NOT_SYSTEM | GDT_SEG_DATA_RDWR | GDT_SEG_ACCESSED,
        .flags = GDT_SEG_GRAN | GDT_SEG_LONG,
        .limit_high = 0xf,
        .base_high = 0,
    };

    // TSS
    *(gdt_system_segment *)(&gdt[3]) = (gdt_system_segment){
        .limit_low = 0x68,
        .base_low = 0,
        .access = GDT_SEG_PRES | GDT_SEG_DESCTYPE_NOT_SYSTEM | GDT_SEG_SIZE_32,
        .flags = 0xc,
        .limit_high = 0xf,
        .base_mid = 0,
        .base_high = 0,
    };
}

void get_memory_map(range_Range **resulting_memory_map, uint64_t *resulting_memory_map_size)
{
    range_Range *memory_map = (void *)MEMORY_MAP_PHYSICAL_ADDR;
    uint64_t memory_map_length = 0;

    printf("[*] Retrieving memory map\n");
    bios_memory_MapEntry entry = {0};
    bios_memory_MapRequestState state = {0};
    while (true)
    {
        bool success = bios_memory_get_mem_map(&entry, &state);
        assert(success && "bios_memory_get_mem_map");
        if (state.done)
            break;

        if (entry.region_length == 0)
            continue;

        if (memory_map_length >= MEMORY_MAP_MAX_LENGTH)
        {
            // If we hit the limit, let's try to defragment, maybe there are some non-merged adjacent ranges
            memory_map_length = range_defragment(memory_map, memory_map_length);
            if (memory_map_length >= MEMORY_MAP_MAX_LENGTH)
            {
                // Didn't work
                printf(
                    "[WARNING] There's not enough space for the memory map ranges. "
                    "Either there's a bug in the kernel, your bios, or you actually "
                    "have more than %lld usable memory ranges. Skipping all following memory ranges.\n",
                    MEMORY_MAP_MAX_LENGTH);
                break;
            }
        }

        uint64_t begin = entry.base_address;
        uint64_t size = entry.region_length;
        uint64_t end = begin + size;
        printf("    %llx - %llx (%x)\n", begin, end, entry.type);

        if (entry.type == bios_memory_TYPE_USABLE)
        {
            memory_map[memory_map_length] = (range_Range){
                .begin = begin,
                .size = size,
            };
            memory_map_length++;
        }
    }

    memory_map_length = range_defragment(memory_map, memory_map_length);

    printf("[*] Available:\n");
    // Both print out, and page-align all the ranges
    for (int i = 0; i < memory_map_length; i++)
    {
        uint64_t begin = PAGE_ALIGN_UP(memory_map[i].begin);
        uint64_t size = PAGE_ALIGN_DOWN(memory_map[i].size);
        memory_map[i].begin = begin;
        memory_map[i].size = size;
        uint64_t end = begin + size;
        printf("    %llx - %llx\n", begin, end);
    }

    // Temporarily, remove all the pages the bootloader occupies
    for (int64_t i = 0; i < memory_map_length; i++)
    {
        // HACK: because the bootloader starts at physical page 0, we can assume that the first entry in the range will be of our bootloader
        if (memory_map[i].begin > (uint32_t)&__end)
            break;

        uint64_t range_end = memory_map[i].begin + memory_map[i].size;
        bool is_end_in_range = range_end > (uint32_t)&__end;
        if (is_end_in_range)
        {
            memory_map[i].begin = PAGE_ALIGN_UP((uint32_t)&__end);
            bool overflow = __builtin_sub_overflow(range_end, memory_map[i].begin, &memory_map[i].size);
            assert(!(!overflow && memory_map[i].size < PAGE_SIZE) && "Because all sizes and pages are aligned, this case should be impossible. If you see this error, please report it");
            bool is_range_empty = overflow;
            if (!is_range_empty)
            {
                break;
            }
            // Otherwise, fall-through to the delete
        }

        // Delete current range
        // HACK: Because the memory_map map is moved to a different memory location later anyway, and because we know the ranges we need start at 0, to delete an entry we can just shift the beginning of the map like so
        memory_map++;
        i--;
    }

    assert(memory_map_length != 0 && "Out of memory");
    range_Range *last_map = &memory_map[memory_map_length - 1];
    range_Range *new_memory_map_addr = (void *)(last_map->begin + last_map->size - PAGE_SIZE);
    if (last_map->size > PAGE_SIZE)
    {
        last_map->size -= PAGE_SIZE;
    }
    else
    {
        // Remove the range from the map
        memory_map_length--;
    }

    memmove(new_memory_map_addr, memory_map, memory_map_length * sizeof(*memory_map));
    memory_map = new_memory_map_addr;

    *resulting_memory_map = memory_map;
    *resulting_memory_map_size = memory_map_length;
}
