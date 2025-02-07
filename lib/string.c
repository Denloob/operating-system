#include "string.h"
#include "assert.h"
#include <stddef.h>

int strlen(const char *s)
{
    int len = 0;
    while (*s++)
    {
        len++;
    }

    return len;
}


char *strchr(const char *str, char search_str)
{
    while (*str)
    {
        if (*str == search_str)
        {
            return (char *)str;
        }
        str++;
    }
    if (search_str == '\0')
    {
        return (char *)str;
    }

    return NULL;
}

char *strncpy(char *dest, const char *src, size_t n)
{
    size_t i = 0;

    for (; i < n && src[i] != '\0'; i++)
    {
        dest[i] = src[i];
    }
    for (; i < n; i++)
    {
        dest[i] = '\0';
    }

    return dest;
}

char *strcpy(char *dest, const char *src)
{
    assert(dest && src);
    while ((*dest++ = *src++))
    {
    }

    return dest;
}

int tolower(int c)
{
    if (c >= 'A' && c <= 'Z')
    {
        return c | ('a' - 'A');
    }

    return c;
}

int strcasecmp(const char *s1, const char *s2)
{
    int result = 0;

    if (s1 == s2)
        return 0;

    while (( result = tolower(*s1) - tolower(*s2++)) == 0)
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

    while (( result = tolower(*s1) - tolower(*s2++)) == 0)
    {
        if (*s1++ == '\0' || --n == 0)
            break;
    }

    return result;
}


int strcmp(const char *str1, const char *str2)
{
    while (*str1 && (*str1 == *str2))
    {
        str1++;
        str2++;
    }
    return *(unsigned char *)str1 - *(unsigned char *)str2;
}

int all_chars_are_same(const char *str)
{
    size_t length = strlen(str);
    char first_char = str[0];
    for (size_t i = 1; i < length; i++)
    {
        if (str[i] != first_char)
        {
            return 0;
        }
    }
    return 1;
}
