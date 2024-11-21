#pragma once

#include <stdint.h>

// GDT utility macros. Refer to Intel documentation for more info.

// Flags
#define GDT_SEG_LONG      (1 << 0x1) // Long mode
#define GDT_SEG_SIZE_32      (1 << 0x2) // Size (0 for 16-bit, 1 for 32)
#define GDT_SEG_GRAN      (1 << 0x3) // Granularity (0 for 1B - 1MB, 1 for 4KB - 4GB)
 
// Access Byte
#define GDT_SEG_ACCESSED       0x01
#define GDT_SEG_EXPAND_DOWN    0x04

#define GDT_SEG_DESCTYPE_NOT_SYSTEM  (1 << 0x04) // Descriptor type (0 for system, 1 for code/data)
#define GDT_SEG_PRES      (1 << 0x07) // Present
#define GDT_SEG_PRIV(x)     (((x) &  0x03) << 0x05)   // Set privilege level (0 - 3)

#define GDT_SEG_DATA_RD        0x00 // Read-Only
#define GDT_SEG_DATA_RDWR      0x02 // Read/Write
#define GDT_SEG_CODE_EX        0x08 // Execute-Only
#define GDT_SEG_CODE_EXRD      0x0A // Execute/Read

typedef struct {
    uint64_t limit_low : 16;
    uint64_t base_low : 24;
    uint64_t access : 8;
    uint64_t limit_high : 4;
    uint64_t flags : 4;
    uint64_t base_high : 8;
} gdt_entry;

#define GDT_SYS_SEG_64BIT_TSS 9
typedef struct {
    uint64_t limit_low : 16;
    uint64_t base_low : 24;

    uint64_t type : 4;
    uint64_t reserved1 : 1; // Thanks to this bit being 0, the CPU knows it's a System Segment and not a GDT entry
    uint64_t privilege_level : 2;
    uint64_t present : 1;
    uint64_t limit_high : 4;

    uint64_t available1 : 1;
    uint64_t reserved2 : 1;
    uint64_t reserved3 : 1;
    uint64_t granularity : 1;
    uint64_t base_mid : 8;
    uint64_t base_high : 32;
    uint64_t reserved : 32;
} gdt_system_segment;

typedef struct __attribute__((__packed__)) {
    uint16_t size;
    uint64_t offset;
} gdt_descriptor;
