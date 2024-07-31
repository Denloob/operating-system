#include "io.h"
#include "shell.h"

#define INPUT_BUFFER_SIZE 256 

void __attribute__((section(".entry"))) kernel_main(uint16_t drive_id)
{
    puts("Hello, World!");
    while(true)
    {
      char input_buffer[INPUT_BUFFER_SIZE];
      put("$ ");
      get_string(input_buffer, INPUT_BUFFER_SIZE);

      parse_command(input_buffer, INPUT_BUFFER_SIZE);
    }
}
