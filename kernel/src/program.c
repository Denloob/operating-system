#include "program.h"
#include "kmalloc.h"
#include "memory.h"
#include "pcb.h"
#include "string.h"
#include "FAT16.h"
#include "assert.h"
#include "mmap.h"
#include "res.h"
#include "scheduler.h"

static void *push_to_stack(void *stack_top, uint64_t value)
{
    stack_top -= sizeof(uint64_t);
    memmove(stack_top, &value, sizeof(uint64_t));

    return stack_top;
}

static void *copy_argv_to_stack_and_push_argc(char **argv_to_copy, void *stack_top)
{
    size_t argv_len = 0;
    char **it = argv_to_copy;
    while (*it++)
    {
        argv_len++;
    }

    if (argv_len == 0)
    {
        stack_top = push_to_stack(stack_top, 0); // Push argv end NULL ptr.
        stack_top = push_to_stack(stack_top, 0); // Push argc = 0
        return stack_top;
    }

    char **argv_ptrs = kmalloc(sizeof(uint64_t) * argv_len);
    if (argv_ptrs == NULL)
    {
        return NULL;
    }

    for (size_t i = 0; i < argv_len; i++)
    {
        size_t str_size = strlen(argv_to_copy[i]) + 1; // Include the null byte in the length
        stack_top -= str_size;
        memmove(stack_top, argv_to_copy[i], str_size);
        argv_ptrs[i] = stack_top;
    }

    stack_top = push_to_stack(stack_top, 0); // Push argv end NULL ptr.
    for (size_t len_i = argv_len; len_i > 0; len_i--)
    {
        size_t i = len_i - 1;
        stack_top = push_to_stack(stack_top, (uint64_t)argv_ptrs[i]);
    }

    kfree(argv_ptrs);

    stack_top = push_to_stack(stack_top, argv_len); // Push argc

    return stack_top;
}

res program_setup_from_drive(uint64_t id,  PCB *parent, mmu_PageMapEntry *kernel_pml, fat16_Ref *fat16, const char *path_to_file, char **argv) 
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

    program_pcb->page_break = 0xa110c00000; // TODO: put it right after .bss and .data

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

    program_pcb->regs.rsp = (uint64_t)copy_argv_to_stack_and_push_argc(
        argv, (void *)program_pcb->regs.rsp);

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
