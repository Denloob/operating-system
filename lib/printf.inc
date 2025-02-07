#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>

static void __printf_putstring(char *str)
{
    while (*str)
    {
        putchar(*str);

        str++;
    }
}

#define define_ttoa(name, type)                                                \
char *name(type value_tmp, char *str, size_t size, int base)                   \
{                                                                              \
    assert(size != 0 && "size cannot be 0");                                   \
    unsigned type value = value_tmp;                                           \
    int sign = (value_tmp < 0);                                                \
    if (sign && base == 10)                                                    \
        value = -value_tmp;                                                    \
                                                                               \
    /* Check if base is supported */                                           \
    if (base < 2 || base > 36)                                                 \
    {                                                                          \
        *str = '\0';                                                           \
        return str;                                                            \
    }                                                                          \
                                                                               \
    char *ptr = str;                                                           \
    if (sign && base == 10)                                                    \
    {                                                                          \
        *ptr++ = '-';                                                          \
        size--;                                                                \
    }                                                                          \
    char *number_start = ptr;                                                  \
                                                                               \
    do                                                                         \
    {                                                                          \
        if (size == 0)                                                         \
        {                                                                      \
            return NULL;                                                       \
        }                                                                      \
                                                                               \
        *ptr++ = "0123456789abcdefghijklmnopqrstuvwxyz"[value % base];         \
        size--;                                                                \
                                                                               \
        value /= base;                                                         \
    } while (value);                                                           \
                                                                               \
    if (size == 0)                                                             \
    {                                                                          \
        return NULL;                                                           \
    }                                                                          \
    *ptr-- = '\0';                                                             \
                                                                               \
    /* Reverse the string (the numbers are reversed)  */                       \
    while (number_start < ptr)                                                 \
    {                                                                          \
        char tmp = *number_start;                                              \
        *number_start++ = *ptr;                                                \
        *ptr-- = tmp;                                                          \
    }                                                                          \
    return str;                                                                \
}

define_ttoa(itoa, int)
define_ttoa(ltoa, long)
define_ttoa(lltoa, long long)

void printf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    for (; *fmt; fmt++)
    {
        if (*fmt != '%')
        {
            putchar(*fmt);
            continue;
        }

        fmt++;
        char ch = *fmt;

        bool zero_padding = false;
        int width = 0;

        if( ch == '0')
        {
            zero_padding = true;
            fmt++;
            ch = *fmt;
            width = ch - '0'; //the digit that spesifiy the width (multi digit is not supported)
            fmt++;
            ch = *fmt;
        }

        bool is_long = false;
        bool is_long_long = false;

        // Check for size modifiers (l or ll)
        if (ch == 'l')
        {
            if (*(fmt + 1) == 'l')
            {
                is_long_long = true;
                fmt++;
            }
            else
            {
                is_long = true;
            }
            fmt++;
            ch = *fmt; // Get the actual format character (d/x)
        }

        switch (ch)
        {
            default:
                putchar(ch);
                break;
            case 's':
            {
                char *str = va_arg(args, char *);
                __printf_putstring(str);
                break;
            }
            case 'p':
            {
                is_long_long = true;
                ch = 'x';

                putchar('0');
                putchar('x');
                /* fall through */
            }
            case 'd':
            case 'x':
            {
                // Get the right vaarg argument depending on size specifier
                long long num = is_long_long ? va_arg(args, long long)
                             : is_long      ? va_arg(args, long)
                                            : va_arg(args, int);

#define MAX_NUM_LENGTH 21
                char buf[MAX_NUM_LENGTH] = {0};

                const int base = ch == 'd' ? 10 : 0x10;

                // Use the appropriate conversion based on size and base
                if (is_long_long)
                {
                    lltoa(num, buf, sizeof(buf), base);
                }
                else if (is_long)
                {
                    ltoa(num, buf, sizeof(buf), base);
                }
                else
                {
                    itoa(num, buf, sizeof(buf), base);
                }

                int len = strlen(buf);
                if(zero_padding && width > len)
                {
                    for(int i=0; i < (width - len) ; i++)
                    {
                        putchar('0');
                    }
                }
                __printf_putstring(buf);
                break;
            }
            case 'c':
            {
                int chr = va_arg(args, int);
                putchar(chr);
                break;
            }
        }
    }
    va_end(args);
}
