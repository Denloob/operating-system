#include "io.h"

#define INPUT_BUFFER_SIZE 256 

void __attribute__((section(".entry"))) kernel_main(uint16_t drive_id)
{
    puts("Hello, World!");
    while(true)
    {
      char input_buffer[INPUT_BUFFER_SIZE];
      puts("Enter a string: ");
      get_string(input_buffer, INPUT_BUFFER_SIZE);

      puts("You entered: ");
      puts(input_buffer);
    }
}
