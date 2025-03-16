#include <stdlib.h>

#define abs_impl(n) ((n) < 0 ? -(n) : (n))

int abs(int num)
{
    return abs_impl(num);
}

long labs(long num)
{
    return abs_impl(num);
}

long long llabs(long long num)
{
    return abs_impl(num);
}
