#pragma once

#include <stddef.h>

int strlen(const char *s);

char* strchr(const char* str, char search_str);
char* strncpy(char *dest, const char *src, size_t n);
char* strcpy(char *dest, const char *src);

int strcasecmp(const char *s1, const char *s2);
int strncasecmp(const char *s1, const char *s2, size_t n);

int strcmp(const char *str1, const char *str2);

int all_chars_are_same(const char *str) ;
