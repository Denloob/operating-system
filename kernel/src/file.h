#pragma once

#include "FAT16.h"
#include "types.h"

typedef struct {
    fat16_File file;
    uint64_t offset;
} FILE;

typedef enum {
    SEEK_SET,
    SEEK_CUR,
    SEEK_END,
} file_Whence;

size_t fread(void *ptr, size_t size, size_t count, FILE *stream);
size_t fwrite(void *ptr, size_t size, size_t count, FILE *stream);
int fseek(FILE *stream, uint64_t offset, file_Whence whence);
long ftell(FILE *stream);
