#include "io.h"

void __attribute__((section(".entry"))) kernel_main(uint16_t drive_id)
{
    puts("Hello, World!");
}
