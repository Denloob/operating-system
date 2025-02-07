#pragma once

#include <stddef.h>
#include <stdint.h>
#include "math.h"

#define PAGE_SIZE 0x1000
#define PAGE_ALIGN_UP(address)   math_ALIGN_UP(address, PAGE_SIZE)
#define PAGE_ALIGN_DOWN(address) math_ALIGN_DOWN(address, PAGE_SIZE)

void *memmove(void *dest, const void *src, int len);    
int memcmp(const void *ptr1, const void *ptr2, size_t num);
void *memset(void *dest, int val, int len);
void *memchr(const void *s, int c, size_t n);
void *memrchr(const void *s, int c, size_t n);

__attribute__((always_inline))
static inline uint8_t mem_bit_test_set(volatile uint32_t *val, int bit) // NOLINT: ignore false positive on `val` being a non-const pointer
{
    uint8_t ret;
    asm volatile (
        "lock bts %[val], %[bit]\n"
        "setc %[ret]\n"
        : [ret] "=r"(ret), [val] "+m"(*val)
        : [bit] "r"(bit)
    );

    return ret;
}

