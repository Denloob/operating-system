#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct Drive {
    uint8_t id;
    uint8_t type;

    uint8_t heads;
    uint8_t sectors;
    uint16_t cylinders;
} Drive;


// WARNING: Do not change the offsets or size of struct's members without changing the bios.asm load_chs_into_registers procedure.
typedef struct drive_CHS
{
    uint16_t cylinder;
    uint8_t head;
    uint8_t sector;
} __attribute__((packed)) drive_CHS;

void drive_lba_to_chs(Drive *drive, uint32_t lba, drive_CHS *ret);

bool drive_init(Drive *drive, uint16_t drive_id);

bool drive_read(Drive *drive, uint64_t address, uint8_t *buffer, uint32_t size);
