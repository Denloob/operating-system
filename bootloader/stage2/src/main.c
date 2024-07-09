
void putc(char ch);
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

void _cdecl cstart_(short param)
{
    puts("Hello, World!\r\n");
}
