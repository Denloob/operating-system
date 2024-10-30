#pragma once

#include <stdint.h>

typedef enum {
    pic_IRQ_TIMER,
    pic_IRQ_KEYBOARD,
    pic_IRQ__CASCADE, // Never raised, for internal use only
    pic_IRQ_COM2,
    pic_IRQ_COM1,
    pic_IRQ_LPT2_OR_SPURIOUS, // When IRQ is aborted, a spurious (last IRQ in the PIC1 or PIC2) is sent
    pic_IRQ_FLOPPY_DISK,
    pic_IRQ_CMOS_CLOCK,
    pic_IRQ_FREE9,
    pic_IRQ_FREE10,
    pic_IRQ_FREE11,
    pic_IRQ_PS2_MOUSE,
    pic_IRQ_FPU,
    pic_IRQ_PRIMARY_ATA,
    pic_IRQ_SECONDARY_ATA_OR_SPURIOUS,
} pic_IRQ ;

/**
 * @brief - Send an end-of-interrupt to the PIC.
 *
 * @param interrupt_request - the number of the interrupt request (IRQ) on the PIC in range [0,15] (inclusive)
 */
void pic_send_EOI(pic_IRQ interrupt_request);

/**
 * @brief - Remap the master PIC (1) and slave PIC (2) IRQ mappings to `offset1` and `offset2`
 *
 * @param offset1 The amount by which to offset the PIC1 IRQs
 * @param offset2 The amount by which to offset the PIC2 IRQs
 */
void PIC_remap(int offset1, int offset2);
