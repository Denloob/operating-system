#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "memory.h"

typedef volatile struct {
    uint32_t lock;
} Mutex;

__attribute__((always_inline))
static inline bool mutex_trylock(Mutex *mutex)
{
    return mem_bit_test_set(&mutex->lock, 0) == 0;
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
