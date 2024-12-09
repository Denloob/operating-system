#include "isr.h"
#include "pcb.h"
#include "regs.h"
#include "schedular.h"
#include "usermode.h"

static uint64_t current_process_id;

void schedular_context_switch_to(PCB *pcb)
{
    current_process_id = pcb->id;
    pcb->state = PCB_STATE_RUNNING;
    asm volatile("mov cr3, %0" : : "a"(pcb->paging) : "memory");
    usermode_jump_to((void *)pcb->rip, &pcb->regs);
}

void __attribute__((used, sysv_abi)) schedular_context_switch_from(Regs *regs,
                                                         isr_InterruptFrame *frame)
{
    regs->rflags = frame->flags;
    regs->rsp = frame->rsp;

    // TODO: Get the pcb for current_process_id
    PCB *pcb = NULL;

    pcb->rip = frame->rip;
    pcb->regs = *regs;
    pcb->state = PCB_STATE_READY;

    // TODO: choose next pcb and call schedular_context_switch_to with it
}
