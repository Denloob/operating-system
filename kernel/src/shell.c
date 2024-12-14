#include "memory.h"
#include "string.h"
#include "shell.h"
#include "vga.h"
#include "donut.h"
#include "FAT16.h"
#include "string.h"

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


void ls_command(fat16_Ref *fat16, const char* flag)
{
    //TODO: check that there is -
    fat16_DirReader reader;
    fat16_init_dir_reader(&reader, &fat16->bpb);
    fat16_DirEntry entry;

    while (fat16_read_next_root_entry(fat16->drive, &reader, &entry)) 
    {
        if (entry.filename[0] != 0x00 && entry.filename[0] != 0xE5)
        {
            char filename[13] = {0};
            strncpy(filename, (char *)entry.filename, sizeof(entry.filename));
            filename[strlen(filename)] = ' ';
            strncpy(filename + strlen(filename), (char *)entry.extension, sizeof(entry.extension));
            if (strchr(flag, 'l'))
            {
                printf("%s  %d bytes\n", filename, entry.fileSize);
            }
            else
            {
                printf("%s\n", filename);
            }
        }
    }
}


void touch_command(fat16_Ref *fat16 ,const char* second_part_command)
{
    fat16_create_file(fat16 , second_part_command);
}
    
void parse_command(char* command , int max_size , fat16_Ref *fat16)
{
    const char *space_pos = strchr(command , ' ');
    const char *second_part = NULL;
    //if there is no space the second part will stay NULL
    if (space_pos)
    {
        second_part = space_pos + 1;
    }

    if (compare_strings("shutdown", command))
    {
        shutdown();
    }
    else if (compare_strings("donut", command))
    {
        donut();
    }
    else if (compare_strings("reboot", command))
    {
        reboot();
    }
    else if (compare_strings("echo", command))
    {
        echo_command(command);
    }
    else if (compare_strings("clear", command))
    {
        clear_command();
    }
    else if (compare_strings("color", command))
    {
        color_command();
    }
    else if (compare_strings("ls", command))
    {
        //if the second part is NULL the passed second part will be a string with no flags
        ls_command(fat16, second_part ? second_part : "-");
    }
    else if (compare_strings("touch", command))
    {
        if (!second_part)  
        {
            puts("touch expects a second argument: filename");
        }
        else
        {
            touch_command(fat16, second_part);
        }
    }
    else
    {
        puts("command does not exist");
    }
}
