#pragma once

#include "FAT16.h"
#include <stddef.h>

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

// fwrite / fread when the request comes from a process.
//  They allow us to put the process in the IO list and not block execution
size_t process_fread(void *ptr, size_t size, size_t count, FILE *stream);
size_t process_fwrite(void *ptr, size_t size, size_t count, FILE *stream);

int fseek(FILE *stream, int64_t offset, file_Whence whence);
long ftell(FILE *stream);
