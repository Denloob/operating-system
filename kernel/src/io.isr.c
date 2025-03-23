#include "io.h"
#include "io.isr.h"
#include "isr.h"
#include "pic.h"
#include "smartptr.h"
#include "scheduler.h"
#include "window.h"

// @see io.c
#define KEYBOARD_PORT 0x60

volatile io_Key io_keyboard_buffer[IO_KEYBOARD_BUFFER_SIZE];
volatile uint8_t io_keyboard_buffer_head;
volatile uint8_t io_keyboard_buffer_tail;

#define TAB_KEY 0xf

struct {
    bool is_rshift_pressed;
    bool is_lshift_pressed;
} g_keyboard_state;

static void __attribute__((used, sysv_abi)) io_isr_keyboard_event_impl(isr_InterruptFrame *frame)
{
    defer({ pic_send_EOI(pic_IRQ_KEYBOARD); });

    int key = in_byte(KEYBOARD_PORT);

    switch (key)
    {
        default:
            break;

        case IO_KEYCODE_LSHIFT:
            g_keyboard_state.is_lshift_pressed = true;
            return;
        case IO_KEYCODE_LSHIFT | IO_KEYCODE_RELEASE_BITMASK:
            g_keyboard_state.is_lshift_pressed = false;
            return;

        case IO_KEYCODE_RSHIFT:
            g_keyboard_state.is_rshift_pressed = true;
            return;
        case IO_KEYCODE_RSHIFT | IO_KEYCODE_RELEASE_BITMASK:
            g_keyboard_state.is_rshift_pressed = false;
            return;
    }

    if (!(get_key_type(key) & KCT_IS_PRESS))
    {
        return;
    }

    if (key == TAB_KEY)
    {
        window_switch_focus();
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


    uint8_t key_modifiers = (int)g_keyboard_state.is_rshift_pressed | (int)g_keyboard_state.is_lshift_pressed;
    io_keyboard_buffer[io_keyboard_buffer_head] = (io_Key){key, key_modifiers};
    io_keyboard_buffer_head++;
    io_keyboard_buffer_head %= IO_KEYBOARD_BUFFER_SIZE;
}

void __attribute__((naked)) io_isr_keyboard_event()
{
    isr_IMPL_INTERRUPT(io_isr_keyboard_event_impl)
}
