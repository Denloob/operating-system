#pragma once

#include <stddef.h>

typedef unsigned char Bitmap;

void bitmap_set(Bitmap *bitmap, int bit);
void bitmap_clear(Bitmap *bitmap, int bit);
int bitmap_test(const Bitmap *bitmap, int bit);
void bitmap_init(Bitmap *bitmap, size_t size);
