#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int id;
} Drive;

// Let know code that includes this code that it supports unaligned addresses and sizes
#define DRIVE_SUPPORTS_UNALIGNED

#define DRIVE_SUPPORTS_VERBOSE

bool drive_init(Drive *drive, int drive_id);

uint64_t drive_read_verbose(Drive *drive, uint64_t address, uint8_t *buffer, uint32_t size);
uint64_t drive_write_verbose(Drive *drive, uint64_t address, const uint8_t *buffer, uint32_t size);
bool drive_read(Drive *drive, uint64_t address, uint8_t *buffer, uint32_t size);
bool drive_write(Drive *drive, uint64_t address, const uint8_t *buffer, uint32_t size);
