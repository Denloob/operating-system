#include "isr.h"
#include "pit.h"
#include "pcb.h"
#include "regs.h"
#include "scheduler.h"
#include "usermode.h"

static PCB *g_current_process;

void scheduler_context_switch_to(PCB *pcb)
{
    g_current_process = pcb;
    pcb->state = PCB_STATE_RUNNING;
    asm volatile("mov cr3, %0" : : "a"(pcb->paging) : "memory");
    usermode_jump_to((void *)pcb->rip, &pcb->regs);
}

void scheduler_context_switch_from(Regs *regs, isr_InterruptFrame *frame)
{
    regs->rflags = frame->flags;
    regs->rsp = frame->rsp;

    PCB *pcb = g_current_process;

    pcb->rip = frame->rip;
    pcb->regs = *regs;
    pcb->state = PCB_STATE_READY;

    // TODO: choose next pcb and call scheduler_context_switch_to with it
}

void scheduler_enable()
{
    pit_enable_special_event();
}

void scheduler_disable()
{
    pit_disable_special_event();
}
