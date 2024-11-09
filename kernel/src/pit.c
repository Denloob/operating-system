#include "pit.h"
#include "io.h"
#include <stdint.h>

#define PIT_CHANNEL_0 0x40
#define PIT_CHANNEL_1 0x41
#define PIT_CHANNEL_2 0x42
#define PIT_COMMAND   0x43

#define PIT_FREQUENCY_HZ (3579545 / 3) // 1.1931816666 (666 repeating) MHz in Hz
#define TARGET_FREQUENCY_HZ 250 // 4 ms in Hz

#define PIT_DIVIDER (PIT_FREQUENCY_HZ / TARGET_FREQUENCY_HZ) // The reload value for the PIT aka the divider

volatile uint64_t g_pit_ms_counter;

void __attribute__((naked)) pit_isr_clock()
{
    asm volatile("push rax\n"

                 "add %[ms_counter], 4\n"

                 "mov al, 0x20\n" // Send EOI to PIC
                 "out 0x20, al\n"

                 "pop rax\n"
                 "iretq\n"
                 : [ms_counter] "+m"(g_pit_ms_counter)
                 :
                 : "memory", "cc");
}


static void set_pit_count(uint16_t count)
{
    out_byte(PIT_CHANNEL_0, count);      // Low byte
    out_byte(PIT_CHANNEL_0, count >> 8); // High byte
}

void pit_init()
{
    cli();
    out_byte(PIT_COMMAND, 0b00110110); // Channel 0, lo/hibyte, mode 3, 16 bit binary

    set_pit_count(PIT_DIVIDER);
    sti();
}
