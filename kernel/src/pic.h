#pragma once

#include <stdint.h>

#define PIC_IDT_OFFSET_1 0x20
#define PIC_IDT_OFFSET_2 0x70

typedef enum {
    pic_IRQ_TIMER,
    pic_IRQ_KEYBOARD,
    pic_IRQ__CASCADE, // Never raised, for internal use only
    pic_IRQ_COM2,
    pic_IRQ_COM1,
    pic_IRQ_LPT2,
    pic_IRQ_FLOPPY_DISK,
    pic_IRQ_LPT1_OR_SPURIOUS, // When IRQ is aborted, a spurious (last IRQ in the PIC1 or PIC2) is sent
    pic_IRQ_CMOS_CLOCK,
    pic_IRQ_FREE9,
    pic_IRQ_FREE10,
    pic_IRQ_FREE11,
    pic_IRQ_PS2_MOUSE,
    pic_IRQ_FPU,
    pic_IRQ_PRIMARY_ATA,
    pic_IRQ_SECONDARY_ATA_OR_SPURIOUS,
} pic_IRQ ;

__attribute__((const, always_inline))
static inline int pic_irq_number_to_idt(int irq)
{
    return irq < pic_IRQ_CMOS_CLOCK
                ? (PIC_IDT_OFFSET_1 + irq)
                : (PIC_IDT_OFFSET_2 + irq - pic_IRQ_CMOS_CLOCK);
}


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

void pic_mask_all();
void pic_clear_mask(pic_IRQ interrupt_request);
void pic_set_mask(pic_IRQ interrupt_request);
