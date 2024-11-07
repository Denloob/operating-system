#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int id;
} Drive;

// Let know code that includes this code that it supports unaligned addresses and sizes
#define DRIVE_SUPPORTS_UNALIGNED

bool drive_init(Drive *drive, int drive_id);

bool drive_read(Drive *drive, uint64_t address, uint8_t *buffer, uint32_t size);
bool drive_write(Drive *drive, uint64_t address, const uint8_t *buffer, uint32_t size);
