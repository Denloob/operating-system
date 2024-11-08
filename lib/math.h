#pragma once

#define math_ALIGN_UP(value, boundary)   (((value) + boundary - 1) & ~(boundary - 1))
#define math_ALIGN_DOWN(value, boundary) ((value) & ~(boundary - 1))

#define MIN(x, y) ((x) > (y) ? (y) : (x))
#define MAX(x, y) ((x) > (y) ? (x) : (y))
