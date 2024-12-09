#pragma once

#include <stdint.h>

/**
 * @brief - Handle the PIT clock interrupt from the PIC
 */
void __attribute__((naked)) pit_isr_clock();

/**
 * @brief - Initialize the pit to precomputed values to work with the clock.
 */
void pit_init();

/**
 * @brief - Get the PIT millisecond counter.
 */
uint64_t pit_ms_counter();

// Enable/disable the PIT from executing a special event every few milliseconds.
//  The special event at the time of writing is context switch.
void pit_enable_special_event();
void pit_disable_special_event();
