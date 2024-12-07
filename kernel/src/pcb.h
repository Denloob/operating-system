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
    uint64_t    id;
    struct PCB *parent;
    Regs        regs;
    pcb_State   state;

    mmu_PageMapEntry *paging;
    // TODO: open file descriptors, signal info
} PCB;
