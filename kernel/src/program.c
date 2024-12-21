#include "program.h"
#include "memory.h"
#include "pcb.h"
#include "FAT16.h"
#include "assert.h"
#include "mmap.h"
#include "scheduler.h"

res program_setup(uint64_t id ,  PCB *parent ,uint64_t program_entry_point , mmu_PageMapEntry *kernel_pml , fat16_Ref *fat16 , const char *path_to_file) 
{
    //pcb handle
    PCB* program_pcb = PCB_init(id, parent , program_entry_point , kernel_pml);

    //fs handling
    FILE file = {0};
    bool success = fat16_open(fat16, path_to_file, &file.file);
    assert(success && "file not found");

    //elf handling
    void *entry_point;
    res rs = elf_load(&file, &entry_point); 
    assert(IS_OK(rs));
    program_pcb->rip = (uint64_t)entry_point; //rip = Register Instruction Pointer and not Rest in peace

    // Virtual memory consts for program stack
    const size_t STACK_SIZE = PAGE_SIZE * 5;
    const uint64_t STACK_VIRTUAL_BASE = 0x7FFFFFFFF000; 
    
    //switch PML
    mmu_load_virt_pml4(program_pcb->paging);

    //Program Stack handling
    program_pcb->regs.rsp = STACK_VIRTUAL_BASE;
    rs = mmap((void *)(program_pcb->regs.rsp - STACK_SIZE), STACK_SIZE, MMAP_PROT_READ | MMAP_PROT_WRITE | MMAP_PROT_RING_3);
    assert(IS_OK(rs));

    // Set initial flags
    program_pcb->regs.rflags = 0x202;
    

    scheduler_process_enqueue(program_pcb);
    scheduler_context_switch_to(program_pcb, SCHEDULER_NOT_A_PIC_INTERRUPT);

    scheduler_enable();

    return res_OK;
}
