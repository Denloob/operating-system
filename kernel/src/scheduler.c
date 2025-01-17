#include "isr.h"
#include "kmalloc.h"
#include "smartptr.h"
#include "pit.h"
#include "pic.h"
#include "pcb.h"
#include "regs.h"
#include "assert.h"
#include "scheduler.h"
#include "usermode.h"
#include "shell.h"

static PCB *g_current_process; // Process queue head
static PCB *g_process_queue_tail;

static PCB *g_io_head; // Process IO linked list

void scheduler_io_push(PCB *pcb, pcb_IORefresh refresh_func)
{
    assert(refresh_func != NULL);

    pcb->refresh = refresh_func;

    pcb->queue_next = g_io_head;
    pcb->queue_prev = NULL;

    if (g_io_head != NULL)
    {
        g_io_head->queue_prev = pcb;
    }
    g_io_head = pcb;
}

void scheduler_io_remove(PCB *pcb)
{
    assert(pcb && g_io_head);

    defer({
        pcb->queue_next = NULL;
        pcb->queue_prev = NULL;
        pcb->refresh = NULL;
    });

    if (pcb == g_io_head)
    {
        g_io_head = pcb->queue_next;
        if (g_io_head)
        {
            g_io_head->queue_prev = NULL;
        }
        return;
    }

    assert(pcb->queue_prev); // There's always prev, as pcb != g_io_head.
    assert(pcb->queue_prev->queue_next == pcb);

    if (!pcb->queue_next)
    {
        pcb->queue_prev->queue_next = NULL;
        return;
    }

    assert(pcb->queue_next->queue_prev == pcb);

    PCB *next = pcb->queue_next;
    PCB *prev = pcb->queue_prev;

    prev->queue_next = next;
    next->queue_prev = prev;
}

void scheduler_io_refresh()
{
    for (PCB *it = g_io_head; it != NULL;)
    {
        PCB *next = it->queue_next;

        assert(it->refresh);
        if (it->refresh(it))
        {
            scheduler_io_remove(it);
            scheduler_process_enqueue(it);
        }

        it = next;
    }
}

PCB *scheduler_current_pcb()
{
    return g_current_process;
}

void scheduler_context_switch_to(PCB *pcb, int pic_number)
{
    g_current_process = pcb;
    
    pcb->state = PCB_STATE_RUNNING;
    mmu_load_virt_pml4(pcb->paging);

    if (pic_number != SCHEDULER_NOT_A_PIC_INTERRUPT) pic_send_EOI(pic_number);

    usermode_jump_to((void *)pcb->rip, &pcb->regs);
}

void scheduler_process_enqueue(PCB *pcb)
{
    pcb->queue_next = NULL;
    if (g_process_queue_tail == NULL)
    {
        g_process_queue_tail = pcb;
    }

    g_process_queue_tail->queue_next = pcb;
    g_process_queue_tail = pcb;
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

    assert(g_process_queue_tail != NULL);
    assert(g_current_process    != NULL);

    PCB *pcb = g_current_process;

    pcb->rip = frame->rip;
    pcb->regs = *regs;
    pcb->state = PCB_STATE_READY;


    PCB *next_pcb = scheduler_get_next_process_and_requeue_current();
    scheduler_context_switch_to(next_pcb, pic_number);
}

void scheduler_process_dequeue_current_and_context_switch()
{
    PCB *next_pcb = g_current_process->queue_next;

    assert(next_pcb != NULL);

    if (next_pcb == g_current_process)
    {
        assert(false && "Shuting down"); // TODO: when we will have an IO queue, we won't want to shutdown
        shutdown();
    }

    // TODO: Unmap all usermode pages by iterating over the mmu struct
    //       Dealloc mmu structures.
    kfree(g_current_process);

    scheduler_context_switch_to(next_pcb, SCHEDULER_NOT_A_PIC_INTERRUPT);
}

void scheduler_enable()
{
    pit_enable_special_event();
}

void scheduler_disable()
{
    pit_disable_special_event();
}

void scheduler_start()
{
    assert(g_process_queue_tail != NULL);

    scheduler_enable();
    scheduler_context_switch_to(g_process_queue_tail, SCHEDULER_NOT_A_PIC_INTERRUPT);
}
