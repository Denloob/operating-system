#include "FAT16.h"
#include "assert.h"
#include "bios.h"
#include "drive.h"
#include "gdt.h"
#include "io.h"
#include "main.h"
#include "mmu.h"

typedef void (*kernel_main)(Drive drive);
#define KERNEL_BEGIN 0x7e00
#define KERNEL_SEGMENT_SIZE 11
#define SEGMENT_SIZE 512
#define KERNEL_SIZE (KERNEL_SEGMENT_SIZE * SEGMENT_SIZE)
#define KERNEL_END (KERNEL_BEGIN + KERNEL_SIZE)
#define KERNEL_BASE_ADDRESS (kernel_main)KERNEL_BEGIN
#define KERNEL_SEGMENT 18

void start(uint16_t drive_id)
{
    Drive drive;
    if (!drive_init(&drive, drive_id))
    {
        assert(false && "drive_init failed");
    }

    fat16_Ref fat16;
    bool success = fat16_ref_init(&fat16, &drive);
    assert(success && "fat16_ref_init");

    fat16_File file;
    success = fat16_open(&fat16, "KERNEL  BIN", &file); //  HACK: fat16 should be case insensitive!
    assert(success && "fat16_open");

    success = fat16_read(&file, (uint8_t *)KERNEL_BEGIN);
    assert(success && "fat16_read");

    mmu_init();

    mmu_map_range(KERNEL_BEGIN, KERNEL_END+0x1000, KERNEL_BEGIN, MMU_READ_WRITE);

    main_gdt_long_mode_init();

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
