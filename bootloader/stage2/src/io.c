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
