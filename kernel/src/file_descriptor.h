#pragma once

#include <stdint.h>
#include "file.h"

typedef struct {
    uint64_t num; // fd number
    FILE file;
    enum {
        file_descriptor_perm_READ  = 0x1,
        file_descriptor_perm_WRITE = 0x2,
        file_descriptor_perm_RW    = file_descriptor_perm_READ | file_descriptor_perm_WRITE,
    } perms;
    bool is_buffered;
} FileDescriptor;
