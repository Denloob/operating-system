#include <stdbool.h>
#include "math.h"

#define COMPARE_PRECISION 1e-15
/**
 * @brief Compare two doubles.
 *
 * @return -1 if first is smaller than second, 1 if second is smaller than first,
 *          0 otherwise. All this withing COMPARE_PRECISION margin.
 */
static int compare_double(double first, double second)
{
    if (second - first > COMPARE_PRECISION)
    {
        return -1;
    }

    if (first - second > COMPARE_PRECISION )
    {
        return 1;
    }

    return 0;
}

double cos(double x)
{
    // cos(x) = cos(-x)
    if (x < 0)
    {
        x = -x;
    }

    // cos(x) = cos(x + 2pi)
    while (compare_double(x, M_PI_M_2) >= 0) // x >= pi * 2
    {
        x -= M_PI_M_2;
    }

    if (compare_double(x,0) == 0)       return 1;
    if (compare_double(x, M_PI_2) == 0) return 0;
    if (compare_double(x, M_PI) == 0)   return -1;

    // cos(x) = cos(2pi - x)
    if (compare_double(x, M_PI) > 0) // x > pi
    {
        x = M_PI_M_2 - x;
    }

    // We got our x to be approximately in range 0 <= x <= pi

    // cos(x) = -cos(pi - x)
    bool flip_sign = false;
    if (compare_double(x, M_PI_2) > 0) // x > pi/2
    {
        x = M_PI - x;
        flip_sign = true;
    }

    // Now x is approximately in range 0 <= x <= pi/2
    // We will get max error of approximately 6.5e-11 (i.e. max value of |actual_cos(x) - our_cos(x)|)

    const double x_squared = x * x;
    const double taylor_series = 1 - (x_squared / 2) * (1 - (x_squared / 12) * (1 - (x_squared / 30) * (1 - (x_squared / 56) * (1 - (x_squared / 90) * (1 - (x_squared / 132) * (1 - (x_squared / 182)))))));
    return flip_sign ? -taylor_series : taylor_series;
    // NOTE: I intentially chose branching over multiplication by `1`/`-1`, as I believe that would by nicer for the CPU. Although this wasn't benchmarked and most likely doesn't have a large impact in any case. If you have any info regarding this, please get in touch with me, I would love to hear what you have to say. - Denloob
}

double sin(double x)
{
    return cos(x - M_PI_2);
}

int isqrt(int x)
{
    // The Babylonian Method
    int a = x;
    int b = 1;
    while (a > b)
    {
        a = (a + b) >> 1;
        b = x / a;
    }
    return a;
}
