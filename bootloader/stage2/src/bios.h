#pragma once

#include <stdbool.h>
#include <stdint.h>

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
