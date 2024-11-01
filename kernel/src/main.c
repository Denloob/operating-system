#include "IDT.h"
#include "error.isr.h"
#include "io.h"
#include "io.isr.h"
#include "io_keyboard.h"
#include "mmu.h"
#include "pic.h"
#include "shell.h"
#include "memory.h"
#include "RTC.h"
#include "PCI.h"
#include "assert.h"
#include "IDE.h"

#define INPUT_BUFFER_SIZE 256 

#define BAR0 0x1F0   // Primary IDE Channel base 
#define BAR1 0x3F6   // Primary IDE Channel control
#define BAR2 0x170   // Secondary IDE Channel base 
#define BAR3 0x376   // Secondary IDE Channel control 
#define BAR4 0x000   // Bus Master IDE 

extern char __bss_start;
extern char __bss_end;
extern char __data_start;
extern char __data_end;
extern char __text_start;
extern char __text_end;
extern char __entry_start;
extern char __entry_end;

static void init_idt();


IDEChannelRegisters channels[2];
ide_device ide_devices[4];

void test_ide();

void __attribute__((section(".entry"))) kernel_main(uint16_t drive_id)
{
    io_clear_vga();

    mmu_map_range((uint64_t)&__entry_start, (uint64_t)&__entry_end, (uint64_t)&__entry_start, MMU_READ_WRITE);
    mmu_map_range((uint64_t)&__text_start, (uint64_t)&__text_end, (uint64_t)&__text_start, 0);
    mmu_map_range((uint64_t)&__bss_start, (uint64_t)&__bss_end, (uint64_t)&__bss_start, MMU_READ_WRITE | MMU_EXECUTE_DISABLE);
    mmu_map_range((uint64_t)&__data_start, (uint64_t)&__data_end, (uint64_t)&__data_start, MMU_READ_WRITE | MMU_EXECUTE_DISABLE);
    memset(&__bss_start, 0, (uint64_t)&__bss_end - (uint64_t)&__bss_start);

    init_idt();

    PIC_remap(0x20, 0x70);
    pic_mask_all();
    pic_clear_mask(pic_IRQ_KEYBOARD);
    idt_register(0x20 + pic_IRQ_KEYBOARD, IDT_gate_type_INTERRUPT, io_isr_keyboard_event);
    io_input_keyboard_key = io_keyboard_wait_key;
    asm volatile ("sti");

    init_memory();

    //PCI scan test
    puts("Starting PCI scan");
    pci_scan_for_ide();
    
    //IDE scan
    puts("Starting IDE scan");
    test_ide();
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
    idt_register(0xe, IDT_gate_type_INTERRUPT, error_isr_page_fault);

    IDTDescriptor idtd = {
        .limit = IDT_LENGTH * sizeof(IDTEntry),
        .base = (uint64_t)g_idt,
    };

    asm volatile("lidt %0" : : "m"(idtd));
}

void test_ide() {
    ide_initialize(BAR0, BAR1, BAR2, BAR3, BAR4);
    for (int i = 0; i < 4; i++) 
    {
        if (ide_devices[i].Reserved)
        {
            printf("IDE Device %d:\n", i);
            printf("  Model: %s\n", ide_devices[i].Model);
            printf("  Type: %s\n", ide_devices[i].Type == IDE_ATA ? "ATA" : "ATAPI");
            printf("  Size: %d sectors\n", ide_devices[i].Size);
            printf("  Channel: %s\n", ide_devices[i].Channel == ATA_PRIMARY ? "Primary" : "Secondary");
            printf("  Drive: %s\n", ide_devices[i].Drive == ATA_MASTER ? "Master" : "Slave");
        }
    }
}
