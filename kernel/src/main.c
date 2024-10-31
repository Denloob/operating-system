#include "IDT.h"
#include "io.h"
#include "mmu.h"
#include "shell.h"
#include "memory.h"
#include "RTC.h"
#include "PCI.h"
#include "assert.h"

#define INPUT_BUFFER_SIZE 256 

extern char __bss_start;
extern char __bss_end;

static void init_idt();

void __attribute__((section(".entry"))) kernel_main(uint16_t drive_id)
{
    io_clear_vga();

    mmu_map_range((uint64_t)&__bss_start, (uint64_t)&__bss_end, (uint64_t)&__bss_start, MMU_READ_WRITE);
    memset(&__bss_start, 0, (uint64_t)&__bss_end - (uint64_t)&__bss_start);

    init_idt();

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

static void init_idt()
{
    g_idt = (void *)0x100; // TODO: TEMP: FIXME: VERY temporary!!!

    memset(g_idt, 0, IDT_LENGTH * sizeof(*g_idt));

    idt_register(0xd, IDT_gate_type_INTERRUPT, error_isr_general_protection_fault);

    IDTDescriptor idtd = {
        .limit = IDT_LENGTH * sizeof(IDTEntry),
        .base = (uint64_t)g_idt,
    };

    asm volatile("lidt %0" : : "m"(idtd));
}
