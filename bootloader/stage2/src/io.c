#include "io.h"

#pragma aux putc =       \
    "mov ah, 0x0E"       \
    "xor bh, bh"         \
    "int 0x10"           \
    modify [ ax bx ]     \
    parm  [ al ];

void puts(char *str)
{
    while (*str)
    {
        putc(*str);
        str++;
    }
}

