#include "pit.h"
#include "io.h"
#include "isr.h"
#include "schedular.h"
#include "smartptr.h"
#include <stdint.h>

#define PIT_CHANNEL_0 0x40
#define PIT_CHANNEL_1 0x41
#define PIT_CHANNEL_2 0x42
#define PIT_COMMAND   0x43

#define PIT_FREQUENCY_HZ (3579545 / 3) // 1.1931816666 (666 repeating) MHz in Hz
#define TARGET_FREQUENCY_HZ 250 // 4 ms in Hz

#define PIT_DIVIDER (PIT_FREQUENCY_HZ / TARGET_FREQUENCY_HZ) // The reload value for the PIT aka the divider

static volatile uint64_t g_pit_ms_counter;

static void __attribute__((used, sysv_abi)) handle_special_time_event_impl()
{
}

static volatile isr_InterruptFrame *g_interrupt_frame_ptr_tmp; // Temp place to store the pointer to isr_InterruptFrame.

static void __attribute__((naked, used)) handle_special_time_event()
{
    // NOTE: we will not return to pit_isr_clock. We are now on our own. Although the timer was already
    // incremented for us.
    // Our special "time event" is a round robin interrupt, so we won't actually iretq back to the same process.
    // Instead we will store all the current process data, and iretq to another process. So the schedular
    // will be responsible for sending IRQ EOI.

    asm volatile(
                "mov %[g_interrupt_frame_ptr_tmp], rsp\n" // We haven't pushed anything yet, so rsp is actually the interrupt frame.
                // 16 byte align the stack
                "sub rsp, 8\n"
                "and rsp, ~0xf\n"

                // Put the `Regs` struct on to the stack. We will pass a pointer to it.

                "push 0\n" // Skip RFALGS, will set them via the isr_InterruptFrame.
                "push r15\n"
                "push r14\n"
                "push r13\n"
                "push r12\n"
                "push r11\n"
                "push r10\n"
                "push r9\n"
                "push r8\n"
                "push rdi\n"
                "push rsi\n"
                "push rbp\n"
                "push 0\n" // Skip rsp, will set them via the isr_InterruptFrame.
                "push rbx\n"
                "push rdx\n"
                "push rcx\n"
                "push rax\n"
                "push 0\n" // Padding before the SSE (stack aligned)

                "sub rsp, 512\n"
                "fxsave [rsp]\n"

                "mov rdi, rsp\n"
                "mov rsi, %[g_interrupt_frame_ptr_tmp]\n"

                "call schedular_context_switch_from" // Does not return. We use call and not jmp to keep the stack 16 byte aligned.
        : [g_interrupt_frame_ptr_tmp] "+m"(g_interrupt_frame_ptr_tmp) : : "memory");
}

void __attribute__((naked)) pit_isr_clock()
{
    asm volatile(
                "add %[ms_counter], 4\n"

                "test %[ms_counter], (2 << 2) - 1\n" // if (ms_counter % 8 == 0)
                "jz handle_special_time_event" // Does not return

                 "push rax\n"

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

uint64_t pit_ms_counter()
{
    return g_pit_ms_counter;
}
