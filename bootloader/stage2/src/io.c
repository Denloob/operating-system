#include "io.h"

#define VGA_ADDR (volatile char *)0xb8000
#define VGA_WIDTH 160
#define VGA_COLOR_WHITE 7
volatile char *g_vga_it = VGA_ADDR;

void puts(char *str)
{
    while (*str)
    {
        *g_vga_it++ = *str++;
        *g_vga_it++ = VGA_COLOR_WHITE;
    }

    // Move g_vga_it to the next line
    g_vga_it += VGA_WIDTH - (g_vga_it - VGA_ADDR) % VGA_WIDTH;
}
