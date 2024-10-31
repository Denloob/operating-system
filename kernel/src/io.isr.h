#pragma once
#include "isr.h"

#define IO_KEYBOARD_BUFFER_SIZE 0x10
extern int io_keyboard_buffer[IO_KEYBOARD_BUFFER_SIZE + 1]; // +1 for ring buffer impl (is-full detection)
// Items are inserted at the head, and popped from tail
extern uint8_t io_keyboard_buffer_head;
extern uint8_t io_keyboard_buffer_tail;

/**
 * @brief - Handle the keyboard interrupt from the PIC
 */
void __attribute__((naked)) io_isr_keyboard_event();
