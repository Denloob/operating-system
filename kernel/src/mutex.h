#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef volatile struct {
    uint32_t lock;
} Mutex;

__attribute__((always_inline))
static inline uint8_t __mutex_bit_test_set(volatile uint32_t *val, int bit) // NOLINT: ignore false positive on `val` being a non-const pointer
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

__attribute__((always_inline))
static inline bool mutex_trylock(Mutex *mutex)
{
    return __mutex_bit_test_set(&mutex->lock, 0) == 0;
}

__attribute__((always_inline))
static inline void mutex_lock(Mutex *mutex)
{
    while (!mutex_trylock(mutex))
    {
        asm ("hlt");
    }
}

__attribute__((always_inline))
static inline void mutex_unlock(Mutex *mutex)
{
    asm ("mov %0, 0" :: "m"(mutex->lock));
}
