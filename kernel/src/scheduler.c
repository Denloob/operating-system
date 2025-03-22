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
#include "string.h"
#include "vga.h"

static PCB *g_current_process; // Process queue head
static PCB *g_process_queue_tail;

static PCB *g_io_head; // Process IO linked list

static PCB *wait_until_one_IO_is_ready()
{
    if (g_io_head == NULL)
    {
        return NULL;
    }

    PCB *pcb = NULL;
    while ((pcb = scheduler_io_refresh()) == NULL)
        continue;

    return pcb;
}

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

PCB *scheduler_io_refresh()
{
    PCB *first_rescheduled = NULL;
    for (PCB *it = g_io_head; it != NULL;)
    {
        PCB *next = it->queue_next;

        assert(it->refresh);
        mmu_load_virt_pml4(it->paging);
        if (it->refresh(it) == PCB_IO_REFRESH_DONE)
        {
            first_rescheduled = it;
            scheduler_io_remove(it);
            scheduler_process_enqueue(it);
        }

        it = next;
    }

    return first_rescheduled;
}

PCB *scheduler_current_pcb()
{
    return g_current_process;
}

void scheduler_context_switch_to(PCB *pcb, int pic_number)
{
    asm volatile("clac" ::: "memory"); // Ensure we are never returning to usermode with AC set.

    if (pcb == NULL)
    {
        pcb = wait_until_one_IO_is_ready();
        if (pcb == NULL)
        {
            assert(false && "No processes left to run; shutdown");
            shutdown();
            assert(false && "Unreachable");
        }
    }
    assert(pcb != NULL);

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

    // While we are here, refresh IO
    scheduler_io_refresh();

    PCB *pcb = g_current_process;

    pcb->rip = frame->rip;
    pcb->regs = *regs;
    pcb->state = PCB_STATE_READY;


    PCB *next_pcb = scheduler_get_next_process_and_requeue_current();
    scheduler_context_switch_to(next_pcb, pic_number);
}

void scheduler_move_current_process_to_io_queue_and_context_switch(pcb_IORefresh refresh_func)
{
    assert(g_current_process != NULL);

    PCB *target = g_current_process;
    PCB *next_pcb = g_current_process->queue_next;

    // It's OK if next_pcb is null.

    if (next_pcb == g_current_process)
    {
        next_pcb = NULL;
    }

    scheduler_io_push(target, refresh_func);

    scheduler_context_switch_to(next_pcb, SCHEDULER_NOT_A_PIC_INTERRUPT);

    assert(false && "Unreachable");
}

void scheduler_process_dequeue_current_and_context_switch()
{
    PCB *next_pcb = g_current_process->queue_next;
    // It's OK if next_pcb is NULL.

    if (next_pcb == g_current_process)
    {
        next_pcb = NULL;
    }

    g_current_process->state = PCB_STATE_ZOMBIE;
    if (g_current_process->parent == NULL)
    {
        PCB_cleanup(g_current_process);
    }

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

PCB *scheduler_find_by_pid(uint64_t pid)
{
    for (PCB *it = g_current_process; it != NULL; it = it->queue_next)
    {
        if (it->id == pid)
        {
            return it;
        }
    }

    for (PCB *it = g_io_head; it != NULL; it = it->queue_next)
    {
        if (it->id == pid)
        {
            return it;
        }
    }

    return NULL;
}

int scheduler_get_all_processes(ProcessInfo *out, int max)
{
    int count = 0;

    PCB *cur = g_current_process;
    while (cur && count < max)
    {
        out[count].pid = cur->id;
        strncpy(out[count].name, cur->cwd, PROCESS_NAME_MAX_LEN);
        strncpy(out[count].cwd, cur->cwd, sizeof(out[count].cwd));
        out[count].state = cur->state;
        count++;

        cur = cur->queue_next;
    }

    PCB *io = g_io_head;
    while (io && count < max)
    {
        out[count].pid = io->id;
        strncpy(out[count].name, io->cwd, PROCESS_NAME_MAX_LEN);
        strncpy(out[count].cwd, io->cwd, sizeof(out[count].cwd));
        out[count].state = io->state;
        count++;

        io = io->queue_next;
    }

    return count;
}
