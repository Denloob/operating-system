#pragma once

#include "file_descriptor.h"
#include "compiler_macros.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    struct {
        FileDescriptor fd;
        bool is_used;
    } *buf;

    size_t capacity;
} FileDescriptorHashmap;

/*
 * @brief - Emplace (or "allocate") a new cell in the hashmap with the key `fd` for a FileDescriptor.
 *
 * @WARN: invalidates all previous iterators.
 *
 * @return - Pointer to the emplaced FileDescriptor. This pointer is an iterator
 *              into the hashmap
 *              WARN: make sure not to access this pointer after this hashmap
 *                      iterators were invalidated.
 *
 *              Will return NULL if emplace failed.
 *
 */
FileDescriptor *file_descriptor_hashmap_emplace(FileDescriptorHashmap *map, uint64_t fd) WUR;

// Returns an iterator into the map for the element whose fd number is `fd`.
// If no matching element is found, returns NULL.
FileDescriptor *file_descriptor_hashmap_get(FileDescriptorHashmap *map, uint64_t fd) WUR;

// If element with `fd` exists, removes it and returns true. Otherwise, false.
bool file_descriptor_hashmap_remove(FileDescriptorHashmap *map, uint64_t fd) WUR;

// Returns true on success and false on failure.
bool file_descriptor_hashmap_init(FileDescriptorHashmap *map) WUR;

void file_descriptor_hashmap_cleanup(FileDescriptorHashmap *map);
