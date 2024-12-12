#include "isr.h"
#include "pit.h"
#include "pic.h"
#include "pcb.h"
#include "regs.h"
#include "assert.h"
#include "scheduler.h"
#include "usermode.h"

static PCB *g_current_process; // Process queue head
static PCB *g_process_queue_tail;

void scheduler_context_switch_to(PCB *pcb, int pic_number)
{
    g_current_process = pcb;
    if (g_process_queue_tail == NULL) // TODO: temp, instead we probably want an init function
    {
        g_process_queue_tail = g_current_process;
    }

    pcb->state = PCB_STATE_RUNNING;
    asm volatile("mov cr3, %0" : : "a"(pcb->paging) : "memory");

    if (pic_number != SCHEDULER_NOT_A_PIC_INTERRUPT) pic_send_EOI(pic_number);

    usermode_jump_to((void *)pcb->rip, &pcb->regs);
}

PCB *scheduler_get_next_process_and_requeue_current()
{
    // When there's only one process is the queue, will always return it.
    g_process_queue_tail->queue_next = g_current_process;
    g_process_queue_tail = g_current_process;
    PCB *next_pcb = g_current_process->queue_next;
    g_current_process->queue_next = NULL;

    assert(next_pcb != NULL);

    return next_pcb;
}

void scheduler_context_switch_from(Regs *regs, isr_InterruptFrame *frame, int pic_number)
{
    regs->rflags = frame->flags;
    regs->rsp = frame->rsp;

    PCB *pcb = g_current_process;

    pcb->rip = frame->rip;
    pcb->regs = *regs;
    pcb->state = PCB_STATE_READY;


    PCB *next_pcb = scheduler_get_next_process_and_requeue_current();
    scheduler_context_switch_to(next_pcb, pic_number);
}

void scheduler_enable()
{
    pit_enable_special_event();
}

void scheduler_disable()
{
    pit_disable_special_event();
}
