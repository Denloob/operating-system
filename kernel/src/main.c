#include "io.h"
#include "shell.h"
#include "memory.h"

#define INPUT_BUFFER_SIZE 256 

void __attribute__((section(".entry"))) kernel_main(uint16_t drive_id)
{

    init_memory();

    void *block1 = kernel_malloc(10);  // allocate 8 kb
    if (block1) {
        puts("Allocated block1 at");
    } else {
        puts("Failed to allocate block1");
    }

    kfree(block1, 8192);  
    puts("Freed block1");
    
    while(true)
    {
      char input_buffer[INPUT_BUFFER_SIZE];
      put("$ ");
      get_string(input_buffer, INPUT_BUFFER_SIZE);

      parse_command(input_buffer, INPUT_BUFFER_SIZE);
    }
}
