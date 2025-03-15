#include <stdlib.h>

static unsigned int g_rand_state = 1;

int rand_r(unsigned int *seed_ptr)
{
    #define MULTIPLIER  1664525U
    #define INCREMENT   1013904223U

    *seed_ptr = MULTIPLIER * (*seed_ptr) + INCREMENT;

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
