#pragma once
#include <stdint.h>

typedef struct
{
    uint64_t rip;
    uint64_t cs;
    uint64_t flags;
    uint64_t rsp;
    uint64_t ss;
} isr_InterruptFrame;
