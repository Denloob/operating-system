#include <stddef.h>

char *itoa(int value, char *str, size_t size, int base);
char *ltoa(long value, char *str, size_t size, int base);
char *lltoa(long long value, char *str, size_t size, int base);

void printf(const char *fmt, ...)
    __attribute__((format(printf, 1, 2)));

