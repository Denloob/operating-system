#include "assert.h"
#include "bios.h"
#include "drive.h"
#include "gdt.h"
#include "io.h"
#include "main.h"
#include "mmu.h"

typedef void (*kernel_main)(Drive drive);
#define KERNEL_BASE_ADDRESS (kernel_main)0x7e00
#define KERNEL_SEGMENT 16

void start(uint16_t drive_id)
{
    Drive drive;
    if (!drive_init(&drive, drive_id))
    {
        assert(false && "drive_init failed");
    }

    if (!drive_read(&drive, KERNEL_SEGMENT, KERNEL_BASE_ADDRESS, 512 * 11))
    {
        assert(false && "Read failed");
    }

    mmu_init();

    gdt_entry *gdt = main_gdt_64bit;
    main_gdt_descriptor->offset = (uint64_t)gdt;

    // Code
    gdt[1] = (gdt_entry){
        .limit_low = 0xffff,
        .base_low = 0,
        .access = GDT_SEG_PRES | GDT_SEG_DESCTYPE_NOT_SYSTEM | GDT_SEG_CODE_EXRD,
        .flags = GDT_SEG_GRAN | GDT_SEG_LONG,
        .limit_high = 0xf,
        .base_high = 0,
    };

    // Data
    gdt[2] = (gdt_entry){
        .limit_low = 0xffff,
        .base_low = 0,
        .access = GDT_SEG_PRES | GDT_SEG_DESCTYPE_NOT_SYSTEM | GDT_SEG_DATA_RDWR,
        .flags = GDT_SEG_GRAN | GDT_SEG_LONG,
        .limit_high = 0xf,
        .base_high = 0,
    };

    // TSS
    *(gdt_system_segment *)(&gdt[3]) = (gdt_system_segment){
        .limit_low = 0x68,
        .base_low = 0,
        .access = GDT_SEG_PRES | GDT_SEG_DESCTYPE_NOT_SYSTEM | GDT_SEG_SIZE_32,
        .flags = 0xcf,
        .limit_high = 0,
        .base_mid = 0,
        .base_high = 0,
    };

    main_long_mode_jump_to(KERNEL_BASE_ADDRESS);
}
