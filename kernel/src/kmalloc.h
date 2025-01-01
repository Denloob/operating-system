#pragma once

#include "compiler_macros.h"
#include <stddef.h>

/**
 * @brief - Release memory allocated by kmalloc. Do not pass pointers not returned
 *              by kmalloc. If `addr` is NULL, does nothing.
 * @see - kmalloc
 *
 * @param addr - The address to free.
 */
void kfree(void *addr);

/**
 * @brief  -Allocates a chunk of memory of size `size`.
 *              On failure, returns `NULL`. Make sure to check for that.
 *
 * @note - Allocated memory must be released with kfree.
 * @see  - kfree
 *
 * @param size - The size of the chunk to allocate.
 * @return - Address of the allocated chunk on success, NULL on failure.
 */
void *kmalloc(size_t size) attribute__kmalloc WUR;

void *kcalloc(size_t amount, size_t size) attribute__kmalloc WUR;
void *krealloc(void *ptr, size_t size) WUR;

void test_kmalloc();
