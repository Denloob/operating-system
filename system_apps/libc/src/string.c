#include "string.h"
#include "assert.h"
#include <stddef.h>
#include <stdint.h>

size_t strlen(const char *s)
{
    int len = 0;
    while (*s++)
    {
        len++;
    }

    return len;
}

size_t strnlen(const char *s, size_t maxlen)
{
    size_t len = 0;
    while (maxlen-- && *s++)
    {
        len++;
    }

    return len;
}

char *strncat(char *s1, const char *s2, size_t n)
{
  char *s = s1;

  /* Find the end of S1.  */
  s1 += strlen(s1);

  size_t ss = strnlen(s2, n);

  s1[ss] = '\0';
  memmove(s1, s2, ss);

  return s;
}

char *strcat(char *dest, const char *src)
{
  strcpy(dest + strlen(dest), src);
  return dest;
}

char *strchrnul(const char *str, char search_str)
{
    while (*str)
    {
        if (*str == search_str)
        {
            return (char *)str;
        }
        str++;
    }

    return (char *)str;
}

char *strchr(const char *str, char search_str)
{
    char *res = strchrnul(str, search_str);
    if (search_str != *res)
    {
        return NULL;
    }

    return res;
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

void *memmove(void *dest, const void *src, size_t n)
{
  char *d = dest;
  const char *s = src;
  if (d < s)
    while (n--)
      *d++ = *s++;
  else
    {
      const char *lasts = s + (n-1);
      char *lastd = d + (n-1);
      while (n--)
        *lastd-- = *lasts--;
    }
  return dest;
}


int memcmp(const void *ptr1, const void *ptr2, size_t n)
{
    const unsigned char *p1 = (const unsigned char *)ptr1;
    const unsigned char *p2 = (const unsigned char *)ptr2;

    for (size_t i = 0; i < n; i++)
    {
        if (p1[i] != p2[i])
        {
            return p1[i] - p2[i];
        }
    }
    return 0;
}

void *memset(void *s, int c, size_t n)
{
  unsigned char *ptr = s;
  while (n-- > 0)
    *ptr++ = c;
  return s;
}

void *memchr(const void *s_in, int c_in, size_t n)
{
    if (n == 0)
        return NULL;

    uint8_t c = c_in;
    const uint8_t *s = s_in;

    while (n--)
    {
        if (*s == c)
            return (void *)s;

        s++;
    }

    return NULL;
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

int strcmp(const char *s1, const char *s2)
{
    while (*s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

int strncmp(const char *s1, const char *s2, size_t n)
{
    int result = 0;

    if (s1 == s2 || n == 0)
        return 0;

    while ((result = *s1 - *s2++) == 0)
    {
        if (*s1++ == '\0' || --n == 0)
            break;
    }

    return result;
}

void *memcpy(void *dest, const void *src, size_t n)
{
    return memmove(dest, src, n);
}
