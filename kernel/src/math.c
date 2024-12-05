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

    const double x_squared = x * x;
    const double taylor_series = 1 - (x_squared / 2) * (1 - (x_squared / 12) * (1 - (x_squared / 30) * (1 - (x_squared / 56) * (1 - (x_squared / 90) * (1 - (x_squared / 132) * (1 - (x_squared / 182)))))));
    return taylor_series;
}

double sin(double x)
{
    return cos(x - M_PI_2);
}
