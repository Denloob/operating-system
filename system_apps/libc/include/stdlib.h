#pragma once

#include <stddef.h>
#include <sys/cdefs.h>

/**
 * @brief - Release memory allocated by malloc. Do not pass pointers not returned
 *              by malloc. If `addr` is NULL, does nothing.
 * @see - malloc
 *
 * @param addr - The address to free.
 */
void free(void *addr);

/**
 * @brief - Allocates a chunk of memory of size `size`.
 *              On failure, returns `NULL`. Make sure to check for that.
 *
 * @note - Allocated memory must be released with free.
 * @see  - free
 *
 * @param size - The size of the chunk to allocate.
 * @return - Address of the allocated chunk on success, NULL on failure.
 */
void *malloc(size_t size) __attribute_malloc _WUR;

void *calloc(size_t amount, size_t size) __attribute_malloc _WUR;
void *realloc(void *ptr, size_t size) __attribute_malloc _WUR;
