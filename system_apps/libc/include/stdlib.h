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
void *malloc(size_t size) __attribute_malloc(free) _WUR;

void *calloc(size_t amount, size_t size) __attribute_malloc(free) _WUR;
void *realloc(void *ptr, size_t size) _WUR;

int rand(void);
void srand(unsigned int seed);
int rand_r(unsigned int *seed_ptr);

int abs(int num) __attribute__((__const__)) _WUR;
long labs(long num) __attribute__((__const__)) _WUR;;
long long llabs(long long num) __attribute__((__const__)) _WUR;;;
