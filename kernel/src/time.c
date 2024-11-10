#include "time.h"
#include "pit.h"

void sleep_ms(const uint64_t delay_ms)
{
    const uint64_t start = pit_ms_counter();
    const uint64_t target = start + delay_ms;

    while (pit_ms_counter() < target)
    {
        asm ("hlt"); // hlt until next interrupt (most likely from the PIT)
    }
}

