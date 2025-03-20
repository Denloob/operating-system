#include "IDT.h"
#include "mouse.h"
#include "char_special_device.h"
#include "mmu_config.h"
#include "fs.h"
#include "rtl8139.h"
#include "scheduler.h"
#include "syscall.h"
#include "tss.h"
#include "kmalloc.h"
#include "test.h"
#include "gdt.h"
#include "mmap.h"
#include "file.h"
#include "FAT16.h"
#include "bmp.h"
#include "kernel_memory_info.h"
#include "drive.h"
#include "error.isr.h"
#include "debug.isr.h"
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
#include "usermode.h"
#include "vga.h"
#include "execve.h"
#include "vga_char_device.h"
#include <stdbool.h>


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
static void init_gdt_and_tss();


IDEChannelRegisters channels[2];
ide_device ide_devices[4];

int ide_init();
static void init_kernel_memory(uint64_t mmu_map_base_address, range_Range *memory_map, uint64_t memory_map_length);
static void init_pic_keyboard_mouse_and_timer();
static void init_drive_devices();
static void print_time();
static void parse_boot_config_and_play_logo();
static void display_boot_logo(int boot_style, const char *video_path);

#define DEBUG_MODE_OFF

void shell(void)
{
    while (true)
    {
        char input_buffer[INPUT_BUFFER_SIZE];
        put("$ ");
        get_string(input_buffer, INPUT_BUFFER_SIZE);
        parse_command(input_buffer, INPUT_BUFFER_SIZE, &g_fs_fat16);
    }
}

void __attribute__((section(".entry"), sysv_abi)) kernel_main(uint32_t param_mmu_map_base_address, uint32_t param_memory_map, uint32_t param_memory_map_length)
{
    uint64_t mmu_map_base_address = param_mmu_map_base_address;
    range_Range *memory_map = (void *)(uint64_t)param_memory_map;
    uint64_t memory_map_length = param_memory_map_length;

    io_clear_vga();

    init_kernel_memory(mmu_map_base_address, memory_map, memory_map_length);

    init_idt();
    syscall_initialize();

    init_pic_keyboard_mouse_and_timer();
    init_drive_devices();

    print_time();

    test_perform_all();

    parse_boot_config_and_play_logo();

    char_special_device_init();
    vga_char_device_init();

    res rs = rtl8139_init();
    assert(IS_OK(rs) && "RTL8139 was not found");
    rtl8139_register_interrupt_handler(pic_IRQ_FREE11);

    usermode_init_smp();

    io_clear_vga();
    rs = execve("/bin/init", NULL, NULL);
    assert(IS_OK(rs) && "Starting the main process failed");

    scheduler_start();
    /*
    while(true)
    {
        char input_buffer[INPUT_BUFFER_SIZE];
        put("$ ");
        get_string(input_buffer,INPUT_BUFFER_SIZE);
        parse_command(input_buffer, INPUT_BUFFER_SIZE , &g_fs_fat16);
    }
    */

}



static void init_idt()
{
    static IDTEntry idt_arr[IDT_LENGTH];

    g_idt = idt_arr;

    idt_register(0xd, IDT_gate_type_INTERRUPT, error_isr_general_protection_fault);
    idt_register(0xe, IDT_gate_type_INTERRUPT, error_isr_page_fault);
    idt_register(0x3, IDT_gate_type_INTERRUPT, debug_isr_int3);

    IDTDescriptor idtd = {
        .limit = IDT_LENGTH * sizeof(IDTEntry),
        .base = (uint64_t)g_idt,
    };

    asm volatile("lidt %0" : : "m"(idtd));
}

static void init_gdt_and_tss()
{
    static gdt_entry gdt[7] = {0}; // null segment, two ring 0 segments, two ring 3 segments, TSS segment (which takes 2 entries)

    // NOTE: the order must be: Kernel Code, Kernel Data, User Data, User Code
    //  otherwise syscall/sysret would not work.
    //  @see 5.8.8 in the Intel® 64 and IA-32 Architectures Software Developer’s Manual, Volume 3A

    // Ring 0 (aka our (kernel) segments).

    // Code (ring 0)
    gdt[1] = (gdt_entry){
        .limit_low = 0xffff,
        .base_low = 0,
        .access = GDT_SEG_PRES | GDT_SEG_DESCTYPE_NOT_SYSTEM | GDT_SEG_PRIV(0) | GDT_SEG_CODE_EXRD | GDT_SEG_ACCESSED,
        .flags = GDT_SEG_GRAN | GDT_SEG_LONG,
        .limit_high = 0xf,
        .base_high = 0,
    };

    // Data (ring 0)
    gdt[2] = (gdt_entry){
        .limit_low = 0xffff,
        .base_low = 0,
        .access = GDT_SEG_PRES | GDT_SEG_DESCTYPE_NOT_SYSTEM | GDT_SEG_PRIV(0) | GDT_SEG_DATA_RDWR | GDT_SEG_ACCESSED,
        .flags = GDT_SEG_GRAN | GDT_SEG_LONG,
        .limit_high = 0xf,
        .base_high = 0,
    };

    // Data (ring 3)
    gdt[3] = (gdt_entry){
        .limit_low = 0xffff,
        .base_low = 0,
        .access = GDT_SEG_PRES | GDT_SEG_DESCTYPE_NOT_SYSTEM | GDT_SEG_PRIV(3) | GDT_SEG_DATA_RDWR | GDT_SEG_ACCESSED,
        .flags = GDT_SEG_GRAN | GDT_SEG_LONG,
        .limit_high = 0xf,
        .base_high = 0,
    };

    // Code (ring 3)
    gdt[4] = (gdt_entry){
        .limit_low = 0xffff,
        .base_low = 0,
        .access = GDT_SEG_PRES | GDT_SEG_DESCTYPE_NOT_SYSTEM | GDT_SEG_PRIV(3) | GDT_SEG_CODE_EXRD | GDT_SEG_ACCESSED,
        .flags = GDT_SEG_GRAN | GDT_SEG_LONG,
        .limit_high = 0xf,
        .base_high = 0,
    };

    static TSS tss = {
        .rsp0 = KERNEL_STACK_BASE,
    };

    gdt_system_segment system_segment = {
        .limit_low = sizeof(TSS) - 1,

        .type = GDT_SYS_SEG_64BIT_TSS,
        .present = 1,

        .base_low =  ((uint64_t)&tss >> 0)  & 0xffffff,
        .base_mid =  ((uint64_t)&tss >> 24) & 0xff,
        .base_high = ((uint64_t)&tss >> 32) & 0xffffffff,
    };
#define TSS_SEGMENT (5*8) // 0x28
    memmove(&gdt[5], &system_segment, sizeof(system_segment));

    gdt_descriptor gdt_desc = {
        .size = sizeof(gdt) - 1,
        .offset = (uint64_t)gdt,
    };
    asm ("lgdt %0" : : "m"(gdt_desc) : "memory");

    asm ("ltr %0" : : "r"((uint16_t)TSS_SEGMENT) : "memory");
}

void init_kernel_memory(uint64_t mmu_map_base_address, range_Range *memory_map, uint64_t memory_map_length)
{
    mmu_init_post_init(mmu_map_base_address);

    init_gdt_and_tss();

    mmu_unmap_bootloader();

    mmap_init(memory_map, memory_map_length);

    mmu_page_range_set_flags(&__entry_start, &__entry_end, 0);
    mmu_page_range_set_flags(&__text_start, &__text_end, 0);
    mmu_page_range_set_flags(&__rodata_start, &__rodata_end, MMU_EXECUTE_DISABLE);
    mmu_page_range_set_flags(&__data_start, &__data_end, MMU_READ_WRITE | MMU_EXECUTE_DISABLE);

    uint64_t bss_size = PAGE_ALIGN_UP((uint64_t)&__bss_end - (uint64_t)&__bss_start);
    res rs = mmap(&__bss_start, bss_size, MMAP_PROT_READ | MMAP_PROT_WRITE);
    assert(IS_OK(rs) && "Couldn't find a memory region for kernel bss section");

    const uint64_t vga_address = (uint64_t)VGA_GRAPHICS_BUF1_PHYS;
    const uint64_t vga_size = 32 * PAGE_SIZE; // 0xA0000..0xBFFFF
    mmu_map_range(vga_address, vga_address + vga_size, VGA_GRAPHICS_BUF1, MMU_READ_WRITE | MMU_EXECUTE_DISABLE);
    io_vga_addr_base = VGA_TEXT_ADDRESS;
    io_clear_vga(); // After setting io_vga_addr_base must clear the screen.
    mmu_unmap_range(VGA_GRAPHICS_BUF3_PHYS, VGA_GRAPHICS_BUF3_PHYS_END);

    mmu_tlb_flush_all();

    memset(&__bss_start, 0, (uint64_t)&__bss_end - (uint64_t)&__bss_start);

#define MMU_VIRT_ADDR (void *)0xfffff00000000000

    mmu_migrate_to_virt_mem(MMU_VIRT_ADDR);
}

static void init_pic_keyboard_mouse_and_timer()
{
    PIC_remap(PIC_IDT_OFFSET_1, PIC_IDT_OFFSET_2);
    pic_mask_all();

    pic_clear_mask(pic_IRQ_KEYBOARD);
    idt_register(pic_irq_number_to_idt(pic_IRQ_KEYBOARD), IDT_gate_type_INTERRUPT, io_isr_keyboard_event);
    io_input_keyboard_key = io_keyboard_wait_key;

    pic_clear_mask(pic_IRQ_PS2_MOUSE);
    idt_register(pic_irq_number_to_idt(pic_IRQ_PS2_MOUSE), IDT_gate_type_INTERRUPT, mouse_isr);
    mouse_init();

    pic_clear_mask(pic_IRQ_TIMER);
    idt_register(pic_irq_number_to_idt(pic_IRQ_TIMER), IDT_gate_type_INTERRUPT, pit_isr_clock);
    pit_init();

    assert(IS_OK(io_keyboard_reset_and_self_test()) && "Keyboard self test failed. Reboot and try again.");
    asm volatile ("sti");
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

void init_drive_devices()
{
    //PCI scan test
    puts("Starting PCI scan");
    pci_scan_for_ide();

    //IDE scan
    puts("Starting IDE scan");
    const int drive_id = ide_init();

    fs_init(drive_id);
}

void print_time()
{
    // Get current time
    uint8_t hours, minutes, seconds;
    RTC_get_time(&hours, &minutes, &seconds);

    // Get current date
    uint16_t year;
    uint8_t month, day;
    RTC_get_date(&year, &month, &day);

    //the printf is using zero padding
    printf("Current Time: %d:%02d:%02d\n", hours, minutes, seconds);
    printf("Current Date: %d-%d-%d\n", day, month, year);
}

void display_boot_logo(int boot_style, const char *video_path)
{
    FILE file = {0};

    bool success = fat16_open(&g_fs_fat16, video_path, &file.file);
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

    uint64_t lcg_state = 7507165354683234409; // Just a random number

    volatile uint8_t *vga_memory = VGA_GRAPHICS_ADDRESS;
    uint8_t column_speeds[VGA_GRAPHICS_WIDTH];
    uint32_t timer = 0;

    for (int i = 0; i < VGA_GRAPHICS_WIDTH; ++i)
    {
        column_speeds[i] = 1;
    }

    while (1)
    {
        for (int col = 0; col < VGA_GRAPHICS_WIDTH; ++col)
        {
            for (int step = 0; step < column_speeds[col] / 2; step++)
            {
                // Move the row
                for (int row = VGA_GRAPHICS_HEIGHT - 2; row >= 0; --row)
                {
                    volatile uint8_t *current_pixel = vga_memory + row * VGA_GRAPHICS_WIDTH + col;
                    volatile uint8_t *next_pixel = vga_memory + (row + 1) * VGA_GRAPHICS_WIDTH + col;

                    *next_pixel = *current_pixel;

                    *current_pixel = 0;
                }
            }
        }

        timer++;
        if (timer % 2 == 0)
        {
            for (int i = 0; i < VGA_GRAPHICS_WIDTH; ++i)
            {
                if (column_speeds[i] < 10)
                {
                    column_speeds[i] += lcg_state % (boot_style == 1 ? 3 : 2);
                    lcg_state = (lcg_state * 134775813 + 1);
                }
            }
        }

        sleep_ms(10);

        bool is_whole_screen_black = true;
        for (int i = 0; i < VGA_GRAPHICS_WIDTH * VGA_GRAPHICS_HEIGHT; i++)
        {
            if (vga_memory[i] != 0)
            {
                is_whole_screen_black = false;
            }
        }

        if (is_whole_screen_black)
        {
            break;
        }
    }

    vga_restore_default_color_palette();
    vga_mode_text();
    io_clear_vga();
}

static void parse_boot_config_and_play_logo()
{
    FILE file = {0};
    bool success = fat16_open(&g_fs_fat16, "/boot/conf/kernel.cfg", &file.file);
    assert(success && "fat16_open: kernel.cfg not found");

    success = fseek(&file, sizeof("boot_style=") - 1, SEEK_CUR) == 0;
    assert(success && "kernel.cfg is invalid");
    char boot_style_char;
    assert(fread(&boot_style_char, 1, 1, &file) == 1 && "couldn't read kernel.cfg");

    success = fseek(&file, sizeof("\nenable_boot_logo=") - 1, SEEK_CUR) == 0;
    assert(success && "kernel.cfg is invalid");
    char enable_boot_logo;
    assert(fread(&enable_boot_logo, 1, 1, &file) == 1 && "couldn't read kernel.cfg");

    if (enable_boot_logo != '1')
    {
        return;
    }

    success = fseek(&file, sizeof("\nboot_video=") - 1, SEEK_CUR) == 0;
    assert(success && "kernel.cfg is invalid");
    char boot_video;
    assert(fread(&boot_video, 1, 1, &file) == 1 && "couldn't read kernel.cfg");

    const char *boot_video_path = NULL;
    switch (boot_video)
    {
        case 'a':
            boot_video_path = "/boot/logo/amongos.bmp";
            break;

        case 'c':
            boot_video_path = "/boot/logo/cogs.bmp";
            break;

        case 'p':
        default:
            boot_video_path = "/boot/logo/cogs-par.bmp";
            break;
    }
    printf("%s\n", boot_video_path);
    display_boot_logo(boot_style_char - '0', boot_video_path);
}
