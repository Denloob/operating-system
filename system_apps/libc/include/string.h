#pragma once

#include <stddef.h>

size_t strlen(const char *s);

char* strchr(const char* str, char search_str);
char* strchrnul(const char* str, char search_str);
char* strncpy(char *dest, const char *src, size_t n);
char* strcpy(char *dest, const char *src);

char *strncat(char *s1, const char *s2, size_t n);
size_t strnlen(const char *s, size_t maxlen);
char *strcat(char *dest, const char *src);

int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);

int strcasecmp(const char *s1, const char *s2);
int strncasecmp(const char *s1, const char *s2, size_t n);

void *memset(void *s, int c, size_t n);
void *memmove(void *dest, const void *src, size_t n);
int memcmp(const void *ptr1, const void *ptr2, size_t n);
void *memchr(const void *s, int c, size_t n);
void *memrchr(const void *s, int c, size_t n);
