#include "memory.h"
#include "string.h"
#include "shell.h"
#include "vga.h"

#define ACPI_SHUTDOWN 0x2000

void shutdown()
{
  out_word(0x604, 0x2000);
}

void reboot()
{
    uint8_t good = 0x02;
    while (good & 0x02)
        good = in_byte(0x64);
    out_byte(0x64, 0xFE);
    __asm__ volatile("hlt");
}
int compare_strings(char *option, char *command)
{
    while (*option && *command)
    {
        if (*option != *command)
        {
            return 0;
        }
        option++;
        command++;
    }
    if (*option == '\0')
    {
        return 1; 
    }
    return 0; 
}

void echo_command(char *command)
{
#define ECHO_COMMAND_LEN 4
    if (strlen(command) <= ECHO_COMMAND_LEN + 1 && command[ECHO_COMMAND_LEN] != ' ')
    {
        puts("Usage: echo <string>");
        return;
    }
    puts(command + ECHO_COMMAND_LEN + 1);
}

void clear_command()
{
    io_clear_vga();
}

static void draw_square(int x, int y, int size, int color)
{
    for (int i = 0; i < size; i++)
    {
        memset((uint8_t *)VGA_GRAPHICS_ADDRESS + (y + i)* VGA_GRAPHICS_WIDTH + x, color, size);
    }
}

void color_command()
{
    io_clear_vga();
    const char target_key = 'q';
    printf("To exit the color palette, press '%c'.\n", target_key);
    printf("Press '%c' to enter the palette...", target_key);

    int key;
    while (key_to_char(io_input_keyboard_key()) != target_key);
    vga_mode_graphics();
    memset((uint8_t *)VGA_GRAPHICS_ADDRESS, 0xf, VGA_GRAPHICS_WIDTH*VGA_GRAPHICS_HEIGHT);

    const int square_size = 10;
    const int space = 2;

    for (int x_idx = 0; x_idx < 0x10; x_idx++)
    {
        for (int y_idx = 0; y_idx < 0x10; y_idx++)
        {
            const int x = space + x_idx * (square_size + space);
            const int y = space + y_idx * (square_size + space);
            const uint8_t color = y_idx << 4 | x_idx;

            draw_square(x, y, square_size, color);
        }
    }

    while (key_to_char(io_input_keyboard_key()) != target_key);

    vga_mode_text();
    io_clear_vga();
}

void parse_command(char* command , int max_size)
{
  if(compare_strings("shutdown" , command))
  {
    shutdown();
  }
  else if(compare_strings("reboot" , command))
  {
    reboot();
  }
  else if(compare_strings("echo" , command))
  {
    echo_command(command);
  }
  else if(compare_strings("clear" , command))
  {
    clear_command();
  }
  else if(compare_strings("color" , command))
  {
    color_command();
  }
  else
  {
    puts("command does not exist");
  }
}
