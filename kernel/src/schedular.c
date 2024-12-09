#include "isr.h"
#include "pcb.h"
#include "regs.h"
#include "schedular.h"
#include "usermode.h"

static PCB *g_current_process;

void schedular_context_switch_to(PCB *pcb)
{
    g_current_process = pcb;
    pcb->state = PCB_STATE_RUNNING;
    asm volatile("mov cr3, %0" : : "a"(pcb->paging) : "memory");
    usermode_jump_to((void *)pcb->rip, &pcb->regs);
}

void schedular_context_switch_from(Regs *regs, isr_InterruptFrame *frame)
{
    regs->rflags = frame->flags;
    regs->rsp = frame->rsp;

    PCB *pcb = g_current_process;

    pcb->rip = frame->rip;
    pcb->regs = *regs;
    pcb->state = PCB_STATE_READY;

    // TODO: choose next pcb and call schedular_context_switch_to with it
}
