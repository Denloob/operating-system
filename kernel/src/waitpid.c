#include "waitpid.h"
#include "pcb.h"
#include "kmalloc.h"
#include "scheduler.h"
#include "usermode.h"
#include "assert.h"

static bool waitpid_oneshot(PCB *pcb, PCB *target_pcb, usermode_mem *wstatus, uint64_t *ret)
{
    if (target_pcb->state != PCB_STATE_ZOMBIE)
    {
        return false;
    }

    *ret = target_pcb->id;
    if (wstatus != NULL)
    {
        res rs = usermode_copy_to_user(wstatus, &target_pcb->return_code, sizeof(target_pcb->return_code));
        if (!IS_OK(rs))
        {
            *ret = -1;
        }
    }

    size_t child_idx = pcb_ProcessChildrenArray_find(&pcb->children, target_pcb->id);
    if (child_idx != pcb->children.length)
    {
        pcb_ProcessChildrenArray_remove(&pcb->children, child_idx);
    }

    PCB_cleanup(target_pcb);

    return true;
}

typedef struct {
    PCB *target_pcb;
    usermode_mem *wstatus;
    int options;
} WaitpidRefreshArgument;

// @see pcb_IORefresh
static pcb_IORefreshResult pcb_refresh_waitpid(PCB *pcb)
{
    WaitpidRefreshArgument *arg = pcb->refresh_arg;
    assert(arg != NULL);

    uint64_t ret;
    bool process_exited = waitpid_oneshot(pcb, arg->target_pcb, arg->wstatus, &ret);
    if (!process_exited)
    {
        return PCB_IO_REFRESH_CONTINUE;
    }

    pcb->regs.rax = ret;
    kfree(arg);
    return PCB_IO_REFRESH_DONE;
}

uint64_t waitpid(uint64_t pid, usermode_mem *wstatus, int options)
{
#define OPTIONS_NO_HANG (1 << 0)
    PCB *current_pcb = scheduler_current_pcb();
    PCB *target_pcb = NULL;

    if (pid == (uint64_t)-1)
    {
        bool found = false;
        pcb_ProcessChildrenArray *children = &current_pcb->children;
        for (size_t i = 0; i < children->length; i++)
        {
            if (children->arr[i].is_used)
            {
                found = true;
                target_pcb = children->arr[i].pcb;
                break;
            }
        }

        if (!found)
        {
            return -1; // Failed
        }
    }

    if (target_pcb == NULL)
    {
        target_pcb = scheduler_find_by_pid(pid);
    }

    if (target_pcb == NULL)
    {
        size_t target_idx = pcb_ProcessChildrenArray_find(&current_pcb->children, pid);
        if (target_idx != current_pcb->children.length)
        {
            target_pcb = current_pcb->children.arr[target_idx].pcb;
        }
    }

    if (target_pcb == NULL)
    {
        return -1; // Failed
    }

    if (options & OPTIONS_NO_HANG)
    {
        uint64_t ret;
        bool success = waitpid_oneshot(current_pcb, target_pcb, wstatus, &ret);
        if (!success)
        {
            return 0;
        }

        return ret;
    }

    WaitpidRefreshArgument *arg = kcalloc(1, sizeof(*arg));
    if (arg == NULL)
    {
        return -1; // Failed
    }

    arg->target_pcb = target_pcb;
    arg->wstatus = wstatus;
    arg->options = options;

    current_pcb->refresh_arg = arg;

    scheduler_move_current_process_to_io_queue_and_context_switch(pcb_refresh_waitpid);
}
