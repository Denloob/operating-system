#pragma once

#include <stddef.h>

void *memmove(void *dest, const void *src, int len);

void *memset(void *dest, int val, int len);

int memcmp(const void *ptr1, const void *ptr2, size_t num);
