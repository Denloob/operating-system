#include "shell.h"

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

void echo_command(char *command, int max_size)
{
    int new_size = max_size - 5;
    char new_command[new_size];

    for (int i = 4; i < max_size && command[i] != '\0'; i++)
    {
        new_command[i - 4] = command[i];
    }
    new_command[new_size - 1] = '\0'; 
    
    puts(new_command);
}

void clean_command(char *command, int max_size)
{
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
    echo_command(command , max_size);
  }
  else if(compare_strings("clean" , command))
  {
    clean_command(command , max_size);
  }
  else
  {
    puts("command does not exist");
  }
}
