#include "io.h"
#include "io.isr.h"
#include "isr.h"
#include "pic.h"
#include "smartptr.h"

// @see io.c
#define KEYBOARD_PORT 0x60

int io_keyboard_buffer[IO_KEYBOARD_BUFFER_SIZE];
uint8_t io_keyboard_buffer_head;
uint8_t io_keyboard_buffer_tail;

static void __attribute__((used, sysv_abi)) io_isr_keyboard_event_impl(isr_InterruptFrame *frame)
{
    defer({ pic_send_EOI(pic_IRQ_KEYBOARD); });

    int key = in_byte(KEYBOARD_PORT);
    if (!(get_key_type(key) & KCT_IS_PRESS))
    {
        return;
    }

    bool is_ringbuffer_full = ((io_keyboard_buffer_head + 1) %
                               IO_KEYBOARD_BUFFER_SIZE) == io_keyboard_buffer_tail;
    if (is_ringbuffer_full)
    {
        // We overwrite oldest data if it's full

        io_keyboard_buffer_tail++;
        io_keyboard_buffer_tail %= IO_KEYBOARD_BUFFER_SIZE;
    }


    io_keyboard_buffer[io_keyboard_buffer_head] = key;
    io_keyboard_buffer_head++;
    io_keyboard_buffer_head %= IO_KEYBOARD_BUFFER_SIZE;
}

void __attribute__((naked)) io_isr_keyboard_event()
{
    isr_IMPL_INTERRUPT(io_isr_keyboard_event_impl)
}
