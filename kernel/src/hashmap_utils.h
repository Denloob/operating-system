#pragma once

#include <stdint.h>

__attribute__((const))
static inline uint64_t hash_u64(uint64_t val)
{
    // Based on https://github.com/h2database/h2database/blob/cc0e45355d78c3ee6711f6bb73eba4f224c4ae18/h2/src/test/org/h2/test/store/CalculateHashConstantLong.java#L75
    val = (val ^ (val >> 30)) * 0xbf58476d1ce4e5b9;
    val = (val ^ (val >> 27)) * 0x94d049bb133111eb;
    val = val ^ (val >> 31);
    return val;
}

