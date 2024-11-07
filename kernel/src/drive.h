#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int id;
} Drive;


bool drive_init(Drive *drive, int drive_id);

bool drive_read(Drive *drive, uint32_t address, uint8_t *buffer, uint32_t size);
bool drive_write(Drive *drive, uint32_t address, const uint8_t *buffer, uint32_t size);
