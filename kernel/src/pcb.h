#pragma once

#include "mmu.h"
#include "fs.h"
#include "regs.h"
#include <stdint.h>

typedef enum
{
    PCB_STATE_TERMINATED,
    PCB_STATE_RUNNING,
    PCB_STATE_WAITING_FOR_IO,
    PCB_STATE_READY,
} pcb_State;


typedef struct PCB PCB;

typedef enum {
    PCB_IO_REFRESH_DONE,
    PCB_IO_REFRESH_CONTINUE,
} pcb_IORefreshResult;

/**
 * @brief - Refresh the IO operation of the PCB. If the operation was
 *            complete, return true.
 *            That is, this function should update the state of the process
 *            in the given PCB, returning true if and only if the process is
 *            ready to get back into the process queue.
 * @param pcb - the PCB to refresh. The PCB is in the IO queue.
 * @return - PCB_IO_REFRESH_DONE if the PCB is ready to be executed. PCB_IO_REFRESH_CONTINUE otherwise.
 */
typedef pcb_IORefreshResult (*pcb_IORefresh)(PCB *pcb);

struct PCB
{
    // Singly-linked list of PCBs. Next element will run after the current one.
    struct PCB *queue_next;
    struct PCB *queue_prev; // Used only in IO doubly-linked list

    uint64_t    id;
    struct PCB *parent;
    Regs        regs;
    uint64_t    rip;
    uint64_t    page_break;
    pcb_State   state;

    char cwd[FS_MAX_FILEPATH_LEN];
    pcb_IORefresh refresh; // Used only in IO doubly-linked list
    void *refresh_arg;

    mmu_PageMapEntry *paging;
    // TODO: open file descriptors, signal info
};

PCB* PCB_init(uint64_t id, PCB *parent, uint64_t entry_point, mmu_PageMapEntry *kernel_pml);


