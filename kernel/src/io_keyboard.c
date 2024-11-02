#pragma once

#include "io.isr.h"
#include "io.h"
#include <stdbool.h>

int io_keyboard_wait_key()
{
    bool is_ringbuffer_empty;
    do
    {
        // NOTE: here we do not disable interrupt flag, because if we are interrupted in the middle of the compare by the keyboard handler (which can only add to the buffer, not remove), best case it's just before we load vars, and we get the var to `true`. Worst case, it's interrupted when we already read the memory, and it will be `false` for one more round. So no need for `sti` and `cli`.
        is_ringbuffer_empty = io_keyboard_buffer_head == io_keyboard_buffer_tail;
    } while (is_ringbuffer_empty);

    cli();
    int key = io_keyboard_buffer[io_keyboard_buffer_tail];
    io_keyboard_buffer_tail++;
    io_keyboard_buffer_tail %= IO_KEYBOARD_BUFFER_SIZE;
    sti();

    return key;
}