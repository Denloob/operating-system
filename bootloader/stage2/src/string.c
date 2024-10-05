#include "string.h"

int strlen(const char *s)
{
    int len = 0;
    while (*s++)
    {
        len++;
    }

    return len;
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

