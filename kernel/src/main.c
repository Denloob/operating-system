#include "IDT.h"
#include "kmalloc.h"
#include "mmap.h"
#include "file.h"
#include "FAT16.h"
#include "bmp.h"
#include "bmp_image.h"
#include "drive.h"
#include "error.isr.h"
#include "io.h"
#include "io.isr.h"
#include "io_keyboard.h"
#include "mmu.h"
#include "pic.h"
#include "pit.h"
#include "shell.h"
#include "memory.h"
#include "RTC.h"
#include "PCI.h"
#include "assert.h"
#include "IDE.h"
#include "time.h"
#include "vga.h"

#define PAGE_SIZE 0x1000
#define PAGE_ALIGN_UP(address)   (((address) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))
#define PAGE_ALIGN_DOWN(address) ((address) & ~(PAGE_SIZE - 1))

#define INPUT_BUFFER_SIZE 256 

#define BAR0 0x1F0   // Primary IDE Channel base 
#define BAR1 0x3F6   // Primary IDE Channel control
#define BAR2 0x170   // Secondary IDE Channel base 
#define BAR3 0x376   // Secondary IDE Channel control 
#define BAR4 0x000   // Bus Master IDE 

extern char __bss_start;
extern char __bss_end;
extern char __rodata_start;
extern char __rodata_end;
extern char __data_start;
extern char __data_end;
extern char __text_start;
extern char __text_end;
extern char __entry_start;
extern char __entry_end;

static void init_idt();


IDEChannelRegisters channels[2];
ide_device ide_devices[4];

int ide_init();

void __attribute__((section(".entry"), sysv_abi)) kernel_main(uint32_t param_mmu_map_base_address, uint32_t param_memory_map, uint32_t param_memory_map_length)
{
    uint64_t mmu_map_base_address = param_mmu_map_base_address;
    range_Range *memory_map = (void *)(uint64_t)param_memory_map;
    uint64_t memory_map_length = param_memory_map_length;
    mmap_init(memory_map, memory_map_length);

    io_clear_vga();

    mmu_init_post_init(mmu_map_base_address);
    mmu_page_range_set_flags(&__entry_start, &__entry_end, 0);
    mmu_page_range_set_flags(&__text_start, &__text_end, 0);
    mmu_page_range_set_flags(&__rodata_start, &__rodata_end, MMU_EXECUTE_DISABLE);
    mmu_page_range_set_flags(&__data_start, &__data_end, MMU_READ_WRITE | MMU_EXECUTE_DISABLE);

    uint64_t bss_size = PAGE_ALIGN_UP((uint64_t)&__bss_end - (uint64_t)&__bss_start);
    res rs = mmap(&__bss_start, bss_size, MMAP_PROT_READ | MMAP_PROT_WRITE);
    assert(IS_OK(rs) && "Couldn't find a memory region for kernel bss section");

    const uint64_t vga_address = (uint64_t)VGA_GRAPHICS_ADDRESS;
    const uint64_t vga_size = 32 * PAGE_SIZE; // 0xA0000..0xBFFFF
    mmu_map_range(vga_address, vga_address + vga_size, vga_address, MMU_READ_WRITE | MMU_EXECUTE_DISABLE);

    mmu_tlb_flush_all();

    memset(&__bss_start, 0, (uint64_t)&__bss_end - (uint64_t)&__bss_start);

    init_idt();

    PIC_remap(0x20, 0x70);
    pic_mask_all();

    pic_clear_mask(pic_IRQ_KEYBOARD);
    idt_register(0x20 + pic_IRQ_KEYBOARD, IDT_gate_type_INTERRUPT, io_isr_keyboard_event);
    io_input_keyboard_key = io_keyboard_wait_key;

    pic_clear_mask(pic_IRQ_TIMER);
    idt_register(0x20 + pic_IRQ_TIMER, IDT_gate_type_INTERRUPT, pit_isr_clock);
    pit_init();

    assert(IS_OK(io_keyboard_reset_and_self_test()) && "Keyboard self test failed. Reboot and try again.");
    asm volatile ("sti");

    //PCI scan test
    puts("Starting PCI scan");
    pci_scan_for_ide();
    
    //IDE scan
    puts("Starting IDE scan");
    const int drive_id = ide_init();

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

    Drive drive;
    drive_init(&drive, drive_id);

    fat16_Ref fat16;
    bool success = fat16_ref_init(&fat16, &drive);
    assert(success && "fat16_ref_init");

    FILE file;
    memset(&file, 0, sizeof(file)); // HACK: GCC tries to optimize `= {0}` with SIMD, but we don't have the FPU initialized, so we get a GPF.

    success = fat16_open(&fat16, "video.bmp", &file.file);
    assert(success && "fat16_open");

    fseek(&file, 0, SEEK_END);
    uint64_t filesize = ftell(&file);
    fseek(&file, 0, SEEK_SET);

    vga_mode_graphics();
    while (ftell(&file) < filesize)
    {
        const uint64_t time_before = pit_ms_counter();

        bmp_draw_from_stream_at(0, 0, &file);

        const uint64_t time_delta = pit_ms_counter() - time_before;
        const uint64_t ms_between_frames = 40;
        if (time_delta < ms_between_frames)
            sleep_ms(ms_between_frames - time_delta);
    }

    sleep_ms(500);
    vga_mode_text();
    io_clear_vga();
    printf("Welcome to ");
    sleep_ms(500);

    char str[] = "AMONG OS\n";
    for (int i = 0; i < sizeof(str); i++)
    {
        sleep_ms(300);
        putc(str[i]);
    }
char *amongus[] = {
    "           S#####@G#%S        \n",
    "        S%#$-SS----S%#S#S     \n",
    "        S#S       SSS*S##S    \n",
    "       S#S   S@S###%%%%###%   \n",
    "       ##^  S##S^       *S%SS \n",
    "  S#%@@#P   X###S          #S \n",
    " S#PSSS#X   S####SG###@@@@### \n",
    " X#X  ##X    S#############$S \n",
    " S#X  ##X     SS%####$%%-S#X  \n",
    " ##^  ##X                X#S  \n",
    " ##   ##X                X##  \n",
    " ##   ##X                X##  \n",
    " %#S  ##X                X#X  \n",
    " S#SS ##X                ##S  \n",
    "  -%####S     S##S@@@@S S##   \n",
    "       ##     ##X S#S^  X#X   \n",
    "       ##     ##X S#S   S#S   \n",
    "       %#G%SS%##^ *S####$S    \n",
    "       *-S%%%%S^\n",
};
    for (int i = 0; i < sizeof(amongus) / sizeof(*amongus); i++)
    {
        sleep_ms(70);
        put(amongus[i]);
    }

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

int ide_init() {
    ide_initialize(BAR0, BAR1, BAR2, BAR3, BAR4);
    int primary_drive_number = -1;
    for (int i = 0; i < 4; i++) 
    {
        if (!ide_devices[i].Reserved)
            continue;

        printf("IDE Device %d:\n", i);
        printf("  Model: %s\n", ide_devices[i].Model);
        printf("  Type: %s\n", ide_devices[i].Type == IDE_ATA ? "ATA" : "ATAPI");
        printf("  Size: %d sectors\n", ide_devices[i].Size);
        printf("  Channel: %s\n", ide_devices[i].Channel == ATA_PRIMARY ? "Primary" : "Secondary");
        printf("  Drive: %s\n", ide_devices[i].Drive == ATA_MASTER ? "Master" : "Slave");
        if (ide_devices[i].Channel == ATA_PRIMARY && ide_devices[i].Size && ide_devices[i].Drive == ATA_MASTER)
        {
            primary_drive_number = i;
        }
    }

    assert(primary_drive_number >= 0 && "Main (IDE) drive was not found");
    printf("[*] Selected device %d: %s\n", primary_drive_number,
           ide_devices[primary_drive_number].Model);
    return primary_drive_number;
}
