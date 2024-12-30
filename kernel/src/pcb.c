#include "pcb.h"
#include "mmu.h"
#include "mmu_config.h"
#include "assert.h"
#include "kmalloc.h"
#include "memory.h"
#include <stdint.h>



PCB* PCB_init(uint64_t id, PCB *parent, uint64_t entry_point, mmu_PageMapEntry *kernel_pml)
{
    const int kernel_start_index = 256;
    const int kernel_end_index = 512;

    PCB* created_pcb = kmalloc(sizeof(struct PCB));
    
    created_pcb->state = PCB_STATE_READY;
    created_pcb->id = id;
    created_pcb->rip = entry_point;
    created_pcb->parent = parent;
    memset(&created_pcb->regs , 0 , sizeof(Regs));
    created_pcb->page_break = UINT64_MAX;

    created_pcb->regs.rflags = 0x202; 

    created_pcb->paging = mmu_map_allocate();
    assert(created_pcb->paging != NULL);
    memset(created_pcb->paging, 0, TABLE_SIZE_BYTES);

    for(int i =kernel_start_index;i<kernel_end_index;i++)
    {
        created_pcb->paging[i] = kernel_pml[i];
    }

    created_pcb->queue_next = NULL;

    return created_pcb;
}

