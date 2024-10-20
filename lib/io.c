#include "io.h"
#include "assert.h"
#include <stdarg.h>
#include "memory.h"
#include "string.h"

#define VGA_ADDR ((volatile char *)0xb8000)
#define VGA_WIDTH 160
#define VGA_HEIGHT 25
#define VGA_COLOR_WHITE 7
#define VGA_SIZE (VGA_WIDTH * VGA_HEIGHT)
volatile char *g_vga_it = VGA_ADDR;

void io_clear_vga()
{
    g_vga_it = VGA_ADDR + VGA_SIZE;
    while (g_vga_it > VGA_ADDR)
    {
        *g_vga_it-- = 0;
    }
}

void scroll_up()
{
    memmove((void *)VGA_ADDR, (const void *)(VGA_ADDR + VGA_WIDTH), VGA_WIDTH * (VGA_HEIGHT - 1));
    memset((void *)(VGA_ADDR + VGA_WIDTH * (VGA_HEIGHT - 1)), 0, VGA_WIDTH);
    g_vga_it -= VGA_WIDTH;
}

void putc(char ch)
{
    if (g_vga_it >= VGA_ADDR + VGA_SIZE)
    {
        scroll_up();
    }

    if (ch == '\b')
    {
        if (g_vga_it - 3 < VGA_ADDR)
        {
            return;
        }
        g_vga_it -= 2;
        *g_vga_it = ' ';
        return;
    }

    if (ch == '\n')
    {
        // Move g_vga_it to the next line
        g_vga_it += VGA_WIDTH - (g_vga_it - VGA_ADDR) % VGA_WIDTH;
        return;
    }

    *g_vga_it++ = ch;
    *g_vga_it++ = VGA_COLOR_WHITE;
}

void put(char *str)
{
    while (*str)
    {
        putc(*str++);
    }
}

void puts(char *str)
{
    put(str);
    putc('\n');
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

void printf(char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    for (; *fmt; fmt++)
    {
        if (*fmt != '%')
        {
            putc(*fmt);
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
                putc(ch);
                break;
            case 's':
            {
                char *str = va_arg(args, char *);
                put(str);
                break;
            }
            case 'd':
            case 'x':
            {
                // Choose the right itoa function depending on size
                long long num = is_long_long ? va_arg(args, long long)
                             : is_long      ? va_arg(args, long)
                                            : va_arg(args, int);

#define MAX_NUM_LENGTH 21
                char buf[MAX_NUM_LENGTH] = {0};

                // Use the appropriate conversion based on size and base
                if (is_long_long)
                {
                    lltoa(num, buf, sizeof(buf), ch == 'd' ? 10 : 0x10);
                }
                else if (is_long)
                {
                    ltoa(num, buf, sizeof(buf), ch == 'd' ? 10 : 0x10);
                }
                else
                {
                    itoa(num, buf, sizeof(buf), ch == 'd' ? 10 : 0x10);
                }

                int len = strlen(buf);
                if(zero_padding && width > len)
                {
                    for(int i=0; i < (width - len) ; i++)
                    {
                        putc('0');
                    }
                }
                put(buf);
                break;
            }
            case 'c':
            {
                int chr = va_arg(args, int);
                putc(chr);
                break;
            }
        }
    }
    va_end(args);
}

// @see https://wiki.osdev.org/PS/2_Keyboard
#define KEYBOARD_PORT 0x60
#define KEYBOARD_PRESS_RELEASE_DELIMETER 0x80

int get_key_type(int keycode)
{
    int res = 0;
    if (keycode < KEYBOARD_PRESS_RELEASE_DELIMETER)
    {
        res |= KCT_IS_PRESS;
    }

    return res;
}

int wait_key()
{
    int k = 0;

    // key press
    while (!(get_key_type(k = in_byte(KEYBOARD_PORT)) & KCT_IS_PRESS))
    {
    }

    // key release
    while (get_key_type(in_byte(KEYBOARD_PORT)) & KCT_IS_PRESS)
    {
    }

    return k;
}

char key_to_char(int keycode)
{
    // @see https://wiki.osdev.org/PS/2_Keyboard#Scan_Code_Set_1
    switch (keycode)
    {
        default:
            return 0;

        case 0x02:
            return '1';
        case 0x03:
            return '2';
        case 0x04:
            return '3';
        case 0x05:
            return '4';
        case 0x06:
            return '5';
        case 0x07:
            return '6';
        case 0x08:
            return '7';
        case 0x09:
            return '8';
        case 0x0a:
            return '9';
        case 0x0b:
            return '0';
        case 0x0c:
            return '-';
        case 0x0d:
            return '=';
        case 0x0e:
            return '\b'; // Backspace
        case 0x0f:
            return '\t'; // Tab
        case 0x10:
            return 'q';
        case 0x11:
            return 'w';
        case 0x12:
            return 'e';
        case 0x13:
            return 'r';
        case 0x14:
            return 't';
        case 0x15:
            return 'y';
        case 0x16:
            return 'u';
        case 0x17:
            return 'i';
        case 0x18:
            return 'o';
        case 0x19:
            return 'p';
        case 0x1a:
            return '[';
        case 0x1b:
            return ']';
        case 0x1c:
            return '\n'; // New line
        case 0x1e:
            return 'a';
        case 0x1f:
            return 's';
        case 0x20:
            return 'd';
        case 0x21:
            return 'f';
        case 0x22:
            return 'g';
        case 0x23:
            return 'h';
        case 0x24:
            return 'j';
        case 0x25:
            return 'k';
        case 0x26:
            return 'l';
        case 0x27:
            return ';';
        case 0x28:
            return '\'';
        case 0x29:
            return '`';
        case 0x2b:
            return '\\';
        case 0x2c:
            return 'z';
        case 0x2d:
            return 'x';
        case 0x2e:
            return 'c';
        case 0x2f:
            return 'v';
        case 0x30:
            return 'b';
        case 0x31:
            return 'n';
        case 0x32:
            return 'm';
        case 0x33:
            return ',';
        case 0x34:
            return '.';
        case 0x35:
            return '/';
        case 0x39:
            return ' ';
    }
}

void get_string(char *buffer, size_t max_size)
{
    char *ptr = buffer;
    char ch;
    int keycode;

    // While loop till "Enter" is pressed
    while (1)
    {
        keycode = wait_key();
        ch = key_to_char(keycode);

        // If the key is a newline character, break the loop
        if (ch == '\n')
        {
            *ptr = '\0';
            putc('\n');
            break;
        }
        // backspace handle
        else if (ch == '\b')
        {
            if (ptr >
                buffer) //not the first key (checks that the ptr is not the buffer so that we dont go before the buffer limit)
            {
                --ptr;
                putc(ch);
            }
        }
        else if (ch) //a case where everything went by normal
        {
            if ((ptr - buffer) <
                max_size - 1) // making sure we are still in the buffer limits
            {
                *ptr++ = ch;
                putc(ch);
            }
        }
    }
}
