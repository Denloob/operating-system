#include "memory.h"
#include <stddef.h>
#include <stdint.h>

void *memmove(void *dest, const void *src, int len)
{
  char *d = dest;
  const char *s = src;
  if (d < s)
    while (len--)
      *d++ = *s++;
  else
    {
      const char *lasts = s + (len-1);
      char *lastd = d + (len-1);
      while (len--)
        *lastd-- = *lasts--;
    }
  return dest;
}

void *memset(void *dest, int val, int len)
{
  unsigned char *ptr = dest;
  while (len-- > 0)
    *ptr++ = val;
  return dest;
}

int memcmp(const void *ptr1, const void *ptr2, size_t num) 
{
    const unsigned char *p1 = (const unsigned char *)ptr1;
    const unsigned char *p2 = (const unsigned char *)ptr2;

    for (size_t i = 0; i < num; i++) 
        {
        if (p1[i] != p2[i]) 
        {
            return p1[i] - p2[i];
        }
    }
    return 0;
}


void *memrchr(const void *s_in, int c_in, size_t n)
{
    if (n == 0)
        return NULL;

    uint8_t c = c_in;
    const uint8_t *s = s_in + n;

    while (n--)
    {
        if (*--s == c)
            return (void *)s;
    }

    return NULL;
}
