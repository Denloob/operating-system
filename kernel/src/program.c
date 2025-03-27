#include "program.h"
#include "kmalloc.h"
#include "smartptr.h"
#include "math.h"
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

    stack_top = (void *)math_ALIGN_DOWN((uint64_t)stack_top, 0x10);

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
    if (program_pcb == NULL)
    {
        return res_program_OUT_OF_MEMORY;
    }

    strncpy(program_pcb->exe_path, path_to_file, sizeof(program_pcb->exe_path));

    bool should_defer_cleanup_pcb = false; // WARN: set to true if and only if program_pcb will not be ever used and can be safely cleaned up

    (void)should_defer_cleanup_pcb; // Used, clang doesn't know that though
    defer({
        if (!should_defer_cleanup_pcb)
        {
            return;
        }

        PCB *pcb = scheduler_current_pcb();
        if (pcb != NULL)
        {
            size_t child_idx = pcb_ProcessChildrenArray_find(&pcb->children, program_pcb->id);
            if (child_idx != pcb->children.length)
            {
                pcb_ProcessChildrenArray_remove(&pcb->children, child_idx);
            }
        }
        PCB_cleanup(program_pcb);
    });

    //switch PML
    mmu_load_virt_pml4(program_pcb->paging);
    defer({
        if (scheduler_current_pcb() != NULL)
        {
            assert(scheduler_current_pcb()->paging != NULL);
            mmu_load_virt_pml4(scheduler_current_pcb()->paging);
        }
    });

    //fs handling
    FILE file = {0};
    bool success = fat16_open(fat16, path_to_file, &file.file);
    if (!success)
    {
        should_defer_cleanup_pcb = true;
        return res_program_GIVEN_FILE_DOESNT_EXIST;
    }

    //elf handling
    void *entry_point;
    res rs = elf_load(&file, &entry_point);
    if (!IS_OK(rs))
    {
        should_defer_cleanup_pcb = true;
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
        should_defer_cleanup_pcb = true;
        return rs;
    }

    program_pcb->regs.rsp = (uint64_t)copy_argv_to_stack_and_push_argc(
        argv, (void *)program_pcb->regs.rsp);

    rs = mprotect(stack_end, STACK_SIZE, MMAP_PROT_READ | MMAP_PROT_WRITE | MMAP_PROT_RING_3);
    if (!IS_OK(rs))
    {
        should_defer_cleanup_pcb = true;
        return rs;
    }

    // Set initial flags
    program_pcb->regs.rflags = 0x202;

    scheduler_process_enqueue(program_pcb);

    return res_OK;
}
