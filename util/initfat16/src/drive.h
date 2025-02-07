#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct Drive {
    FILE *file;
} Drive;


inline bool drive_init(Drive *drive, uint16_t drive_id) { return true; }

bool drive_read(Drive *drive, uint32_t address, uint8_t *buffer, uint32_t size);
