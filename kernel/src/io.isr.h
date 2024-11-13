#pragma once
#include "isr.h"

#define IO_KEYBOARD_BUFFER_SIZE 0x11 // 0x10+1 for ring buffer impl (is-full detection)
extern volatile int io_keyboard_buffer[IO_KEYBOARD_BUFFER_SIZE]; 
// Items are inserted at the head, and popped from tail
extern volatile uint8_t io_keyboard_buffer_head;
extern volatile uint8_t io_keyboard_buffer_tail;

/**
 * @brief - Handle the keyboard interrupt from the PIC
 */
void __attribute__((naked)) io_isr_keyboard_event();
