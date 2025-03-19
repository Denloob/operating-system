#include <stdlib.h>

static unsigned int g_rand_state = 1;

int rand_r(unsigned int *seed_ptr)
{
    unsigned int x = *seed_ptr;

    x ^= (x << 13);
    x ^= (x >> 17);
    x ^= (x << 5);

    *seed_ptr = x;

    return *seed_ptr & ~(1U << 31); // Unset the sign bit before casting to `signed int`
}

int rand()
{
    return rand_r(&g_rand_state);
}

void srand(unsigned int seed)
{
    g_rand_state = seed;
}
