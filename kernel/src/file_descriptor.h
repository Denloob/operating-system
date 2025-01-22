#pragma once

#include <stdint.h>
#include "file.h"

typedef struct {
    uint64_t num; // fd number
    FILE file;
} FileDescriptor;
