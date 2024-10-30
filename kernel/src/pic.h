#pragma once

#include <stdint.h>

/**
 * @brief - Send an end-of-interrupt to the PIC.
 *
 * @param interrupt_request - the number of the interrupt request (IRQ) on the PIC in range [0,15] (inclusive)
 */
void pic_send_EOI(uint8_t interrupt_request);

/**
 * @brief - Remap the master PIC (1) and slave PIC (2) IRQ mappings to `offset1` and `offset2`
 *
 * @param offset1 The amount by which to offset the PIC1 IRQs
 * @param offset2 The amount by which to offset the PIC2 IRQs
 */
void PIC_remap(int offset1, int offset2);
