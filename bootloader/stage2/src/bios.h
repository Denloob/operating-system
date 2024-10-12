#pragma once

#include "drive.h"
#include <stdbool.h>
#include <stdint.h>

#define SECTOR_SIZE 512

/**
 * @brief Get the drive param info
 *
 * @param[in]  drive     The number of the drive to get the params of.
 * @param[out] driveType The drive type (AT/PS2 floppies only).
 * @param[out] cylinders The amount of cylinders on the drive.
 * @param[out] sectors   The amount of sectors on the drive.
 * @param[out] heads     The amount of heads on the drive.
 * @return               true on success, false on failure.
 */
bool __attribute__((cdecl))
bios_drive_get_drive_params(uint8_t drive, uint8_t *driveType,
                            uint16_t *cylinders, uint8_t *sectors,
                            uint8_t *heads);

/**
 * @brief Read from drive
 *
 * @param[in] drive         The number of the drive to read from.
 * @param[in] chs           The CHS address to read.
 * @param[out] buffer       The buffer to read into.
 * @param[int] sector_count The amount of sectors (of size 512 bytes) to read.
 * @return                  true on success, false on failure
 */
bool __attribute__((cdecl))
bios_drive_read(uint8_t drive, drive_CHS *chs, uint8_t *buffer,
                uint8_t sector_count);

enum { 
    bios_memory_TYPE_USABLE,
    bios_memory_TYPE_RESERVED,
    bios_memory_TYPE_ACPI_RECLAIMABLE,
    bios_memory_TYPE_ACPI_NVS,
    bios_memory_TYPE_BAD,
};

typedef struct {
    uint64_t base_address;
    uint64_t region_length;
    uint32_t type;
    uint32_t attr_bitfield;
} __attribute__((__packed__))  bios_memory_MapEntry;

// WARNING: Do not change the offsets or size of struct's members without changing the bios.asm bios_memory_get_mem_map procedure.
typedef struct {
    uint32_t entry_id;    // [in,out] Set to 0 on first call, don't change on subsequent
    uint8_t done;         // [out] If non-zero (true), there are no more entries left
    uint8_t is_extended;  // [out] If non-zero (true), attr_bitfield is present and can be read
    uint16_t _pad;
} __attribute__((__packed__)) bios_memory_MapRequestState;

bool __attribute__((cdecl))
bios_memory_get_mem_map(bios_memory_MapEntry *res, bios_memory_MapRequestState *state);
