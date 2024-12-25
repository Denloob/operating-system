#include "program.h"
#include "memory.h"
#include "pcb.h"
#include "FAT16.h"
#include "assert.h"
#include "mmap.h"
#include "res.h"
#include "scheduler.h"

res program_setup_from_drive(uint64_t id,  PCB *parent, mmu_PageMapEntry *kernel_pml, fat16_Ref *fat16, const char *path_to_file) 
{
    //pcb handle
    PCB* program_pcb = PCB_init(id, parent, 0, kernel_pml);

    //switch PML
    mmu_load_virt_pml4(program_pcb->paging);

    //fs handling
    FILE file = {0};
    bool success = fat16_open(fat16, path_to_file, &file.file);
    assert(success && "file not found");

    //elf handling
    void *entry_point;
    res rs = elf_load(&file, &entry_point); 
    if (!IS_OK(rs))
    {
        return rs;
    }

    program_pcb->rip = (uint64_t)entry_point; //rip = Register Instruction Pointer and not Rest in peace

    // Virtual memory consts for program stack
    const size_t STACK_SIZE = PAGE_SIZE * 5;
    const uint64_t STACK_VIRTUAL_BASE = 0x7FFFFFFFF000; 

    //Program Stack handling
    program_pcb->regs.rsp = STACK_VIRTUAL_BASE;

    void *const stack_end = (void *)(program_pcb->regs.rsp - STACK_SIZE);
    rs = mmap(stack_end, STACK_SIZE, MMAP_PROT_READ | MMAP_PROT_WRITE);
    if (!IS_OK(rs))
    {
        return rs;
    }

    // TODO: actually put argv on the stack. Right now a push argc=0.
    ((uint64_t *)STACK_VIRTUAL_BASE)[-1] = 0;
    program_pcb->regs.rsp -= sizeof(uint64_t);

    rs = mprotect(stack_end, STACK_SIZE, MMAP_PROT_READ | MMAP_PROT_WRITE | MMAP_PROT_RING_3);
    if (!IS_OK(rs))
    {
        return rs;
    }

    // Set initial flags
    program_pcb->regs.rflags = 0x202;

    scheduler_process_enqueue(program_pcb);

    return res_OK;
}


res program_setup_from_code(uint64_t id, PCB *parent, mmu_PageMapEntry *kernel_pml, void (*entry_point)(void)) 
{
    //PCB handle
    PCB *program_pcb = PCB_init(id, parent, 0, kernel_pml);

    //switch PML
    mmu_load_virt_pml4(program_pcb->paging);

    //set rip
    program_pcb->rip = (uint64_t)entry_point;

    // Virtual memory consts for program stack
    const size_t STACK_SIZE = PAGE_SIZE * 5;
    const uint64_t STACK_VIRTUAL_BASE = 0x7FFFFFFFF000;

    //Program stack handling
    program_pcb->regs.rsp = STACK_VIRTUAL_BASE;
    res rs = mmap((void *)(program_pcb->regs.rsp - STACK_SIZE), STACK_SIZE, MMAP_PROT_READ | MMAP_PROT_WRITE | MMAP_PROT_RING_3);
    assert(IS_OK(rs));

    //Set initial flags
    program_pcb->regs.rflags = 0x202;

    scheduler_process_enqueue(program_pcb);

    return res_OK;
}
