#include "bitmap.h"
#include "memory.h"
#include <stddef.h>

void bitmap_set(Bitmap *bitmap, int bit)
{
    bitmap[bit / 8] |= (1 << (bit % 8));
}

void bitmap_clear(Bitmap *bitmap, int bit)
{
    bitmap[bit / 8] &= ~(1 << (bit % 8));
}

int bitmap_test(const Bitmap *bitmap, int bit)
{
    return bitmap[bit / 8] & (1 << (bit % 8));
}

void bitmap_init(Bitmap *bitmap, size_t size)
{
    memset(bitmap, 0, size);
}
