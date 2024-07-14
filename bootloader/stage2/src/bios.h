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
