#include "io.h"
#include "shell.h"
#include "memory.h"
#include "RTC.h"
#include "PCI.h"

#define INPUT_BUFFER_SIZE 256 

void __attribute__((section(".entry"))) kernel_main(uint16_t drive_id)
{
    io_clear_vga();

    init_memory();

    //PCI scan test
    puts("Starting PCI scan");
    pci_scan_for_ide();

    //memory allocation test
    void *block1 = kernel_malloc(10);  // allocate 8 kb

    if (block1) {
        puts("Allocated block1 at");
    } else {
        puts("Failed to allocate block1");
    }

    kfree(block1, 8192);  
    puts("Freed block1");

    
    // Get current time
    uint8_t hours, minutes, seconds;
    get_time(&hours, &minutes, &seconds);
    
    // Get current date
    uint16_t year;
    uint8_t month, day;
    get_date(&year, &month, &day);

    //the printf is using zero padding 
    printf("Current Time: %d:%02d:%02d\n", hours, minutes, seconds);
    printf("Current Date: %d-%d-%d\n", day, month, year);

    while(true)
    {
      char input_buffer[INPUT_BUFFER_SIZE];
      put("$ ");
      get_string(input_buffer, INPUT_BUFFER_SIZE);

      parse_command(input_buffer, INPUT_BUFFER_SIZE);
    }
}
