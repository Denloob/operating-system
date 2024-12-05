#pragma once

#define math_ALIGN_UP(value, boundary)   (((value) + boundary - 1) & ~(boundary - 1))
#define math_ALIGN_DOWN(value, boundary) ((value) & ~(boundary - 1))

#define MIN(x, y) ((x) > (y) ? (y) : (x))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

#define M_PI        3.14159265358979323846	/* pi */
#define M_PI_2      1.57079632679489661923	/* pi/2 */
#define M_PI_M_2    6.28318530717958647692	/* pi*2 */

double cos(double x);
double sin(double x);
