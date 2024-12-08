#include "pcb.h"
#include "usermode.h"
#include "schedular.h"

void schedular_context_switch_to(PCB *pcb)
{
    pcb->state = PCB_STATE_RUNNING;
    asm volatile("mov cr3, %0" : : "a"(pcb->paging) : "memory");
    usermode_jump_to((void *)pcb->rip, &pcb->regs);
}
