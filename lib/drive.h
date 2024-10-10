#pragma once

/* drive.h stub, do not use while compiling, do not include */

#include <stdint.h>
#include <stdbool.h>

typedef struct Drive Drive;


typedef struct drive_CHS drive_CHS;

bool drive_init(Drive *drive, uint16_t drive_id);

bool drive_read(Drive *drive, uint32_t address, uint8_t *buffer, uint32_t size);

