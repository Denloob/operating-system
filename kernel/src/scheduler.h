#pragma once

#include "isr.h"
#include "pcb.h"

#define SCHEDULER_NOT_A_PIC_INTERRUPT -1

/**
 * @brief - Performs a context switch into the process of the given PCB.
 *
 * @param pcb - The PCB of the process to context switch to.
 * @param pic_number - If the request comes from a PIC interrupt, must be set to the
 *                          number of the PIC interrupt. Otherwise, set to SCHEDULER_NOT_A_PIC_INTERRUPT.
 */
void scheduler_context_switch_to(PCB *pcb, int pic_number);

/**
 * @brief - Performs a context switch from the currently running process, with the
 *            registers `regs` and `frame` (which contain the RIP and RSP).
 *          Will context switch to the next process in the process queue.
 *
 * @param regs  - The regs and the SSE state of the process (excluding $rsp and $rip)
 * @param frame - Interrupt frame of the process. Used to get its $rsp and $rip.
 * @param pic_number - If the request comes from a PIC interrupt, must be set to the
 *                          number of the PIC interrupt. Otherwise, set to SCHEDULER_NOT_A_PIC_INTERRUPT.
 */
void scheduler_context_switch_from(Regs *regs, isr_InterruptFrame *frame, int pic_number) __attribute__((used, sysv_abi));

void scheduler_enable();
void scheduler_disable();
