#include "string.h"
#include "types.h"

int strlen(const char *s)
{
    int len = 0;
    while (*s++)
    {
        len++;
    }

    return len;
}

#define TOLOWER(c) (c | ('a' - 'A'))

int strcasecmp(const char *s1, const char *s2)
{
    int result = 0;

    if (s1 == s2)
        return 0;

    while (( result = TOLOWER(*s1) - TOLOWER(*s2++)) == 0)
    {
        if (*s1++ == '\0')
            break;
    }

    return result;
}

int strncasecmp(const char *s1, const char *s2, size_t n)
{
    int result = 0;

    if (s1 == s2 || n == 0)
        return 0;

    while (( result = TOLOWER(*s1) - TOLOWER(*s2++)) == 0)
    {
        if (*s1++ == '\0' || --n == 0)
            break;
    }

    return result;
}
