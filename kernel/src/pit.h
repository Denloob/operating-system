#pragma once

#include <stdint.h>

extern volatile uint64_t g_pit_ms_counter;

/**
 * @brief - Handle the PIT clock interrupt from the PIC
 */
void __attribute__((naked)) pit_isr_clock();

/**
 * @brief - Initialize the pit to precomputed values to work with the clock.
 */
void pit_init();
