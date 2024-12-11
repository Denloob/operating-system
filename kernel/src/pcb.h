#pragma once

#include "mmu.h"
#include "regs.h"
#include <stdint.h>

typedef enum
{
    PCB_STATE_TERMINATED,
    PCB_STATE_RUNNING,
    PCB_STATE_WAITING_FOR_IO,
    PCB_STATE_READY,
} pcb_State;

typedef struct PCB
{
    // Singly-linked list of PCBs. Next element will run after the current one.
    struct PCB *queue_next;

    uint64_t    id;
    struct PCB *parent;
    Regs        regs;
    uint64_t    rip;
    pcb_State   state;

    mmu_PageMapEntry *paging;
    // TODO: open file descriptors, signal info
} PCB;
