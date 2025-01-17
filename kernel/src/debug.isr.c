#include "debug.isr.h"

void __attribute__((naked)) debug_isr_int3()
{
    asm volatile("iretq");
}
