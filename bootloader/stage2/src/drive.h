#pragma once

#include <stdint.h>

typedef struct Drive {
    uint8_t id;
    uint8_t type;

    uint8_t heads;
    uint8_t sectors;
    uint16_t cylinders;
} Drive;


typedef struct drive_CHS {
    uint16_t cylinder;
    uint8_t head;
    uint8_t sector;
} drive_CHS;

void drive_lba_to_chs(Drive *drive, uint32_t lba, drive_CHS *ret);

bool drive_init(Drive *drive, uint16_t drive_id);

bool drive_read(Drive *drive, uint8_t *buffer, uint32_t size);
