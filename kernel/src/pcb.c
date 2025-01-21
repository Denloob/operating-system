#include "pcb.h"
#include "mmu.h"
#include "mmu_config.h"
#include "assert.h"
#include "kmalloc.h"
#include "memory.h"
#include "res.h"
#include <stdint.h>



PCB* PCB_init(uint64_t id, PCB *parent, uint64_t entry_point, mmu_PageMapEntry *kernel_pml)
{
    const int kernel_start_index = 256;
    const int kernel_end_index = 512;

    PCB* created_pcb = kcalloc(1, sizeof(struct PCB));
    if (created_pcb == NULL)
    {
        return NULL;
    }
    
    if (!pcb_ProcessChildrenArray_init(&created_pcb->children))
    {
        kfree(created_pcb);
        return NULL;
    }

    if (parent != NULL)
    {
        memmove(created_pcb->cwd, parent->cwd, sizeof(created_pcb->cwd));
        pcb_ProcessChildrenArray_push(&parent->children, created_pcb);
    }
    else
    {
        created_pcb->cwd[0] = '/'; // NULL terminated because of kcalloc.
    }
    
    created_pcb->state = PCB_STATE_READY;
    created_pcb->id = id;
    created_pcb->rip = entry_point;
    created_pcb->parent = parent;
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

void PCB_cleanup(PCB *pcb)
{
    if (pcb == NULL)
    {
        return;
    }

    // TODO: free the pages

    pcb_ProcessChildrenArray_cleanup(&pcb->children);
    kfree(pcb);
}

bool pcb_ProcessChildrenArray_init(pcb_ProcessChildrenArray *arr)
{
    memset(arr, 0, sizeof(*arr));

    arr->capacity = 2;
    arr->arr = kcalloc(arr->capacity, sizeof(*arr->arr));
    return arr != NULL;
}

void pcb_ProcessChildrenArray_cleanup(pcb_ProcessChildrenArray *arr)
{
    if (arr == NULL)
    {
        return;
    }

    kfree(arr->arr);
    memset(arr, 0, sizeof(*arr));
}

bool pcb_ProcessChildrenArray_get_last_child(pcb_ProcessChildrenArray *arr, PCB **out)
{
    assert(arr->last_child_idx <= arr->length);
    if (arr->last_child_idx == arr->length)
    {
        return false;
    }

    assert(arr->arr[arr->last_child_idx].is_used);
    if (out != NULL)
    {
        *out = arr->arr[arr->last_child_idx].pcb;
    }

    return true;
}

bool pcb_ProcessChildrenArray_have_free_elements(pcb_ProcessChildrenArray *arr)
{
    assert(arr->length <= arr->last_free_idx);
    if (arr->last_free_idx == arr->length)
    {
        return false;
    }
    assert(!arr->arr[arr->last_free_idx].is_used);
    return true;
}

static res pcb_ProcessChildrenArray_grow_arr(pcb_ProcessChildrenArray *arr)
{
    size_t new_capacity = arr->capacity * 2;
    void *new_arr = krealloc(arr->arr, new_capacity * sizeof(*arr->arr));
    if (new_arr == NULL)
    {
        return res_pcb_ProcessChildrenArray_OUT_OF_MEMORY;
    }

    arr->arr = new_arr;
    arr->capacity = new_capacity;

    return res_OK;
}

static void pcb_ProcessChildrenArray_push_to_end(pcb_ProcessChildrenArray *arr, PCB *el)
{
    assert(arr->length < arr->capacity);
    bool has_free_elements = pcb_ProcessChildrenArray_have_free_elements(arr);

    arr->arr[arr->length].pcb = el;
    arr->length++;

    if (!has_free_elements)
    {
        arr->last_free_idx = arr->length;
    }
}

bool pcb_ProcessChildrenArray_needs_growing(pcb_ProcessChildrenArray *arr)
{
    assert(arr->length <= arr->capacity);
    return arr->length == arr->capacity;
}

res pcb_ProcessChildrenArray_push(pcb_ProcessChildrenArray *arr, PCB *el)
{
    if (pcb_ProcessChildrenArray_have_free_elements(arr))
    {
        size_t index = arr->last_free_idx;
        if (arr->arr[index].has_index)
        {
            arr->last_free_idx = arr->arr[index].index;
        }
        else
        {
            arr->last_free_idx = arr->length;
        }

        arr->last_child_idx = index;
        arr->arr[index].pcb = el;
        assert(arr->arr[index].is_used); // Set by setting .pcb
        return res_OK;
    }

    if (pcb_ProcessChildrenArray_needs_growing(arr))
    {
        res rs = pcb_ProcessChildrenArray_grow_arr(arr);
        if (!IS_OK(rs))
        {
            return rs;
        }
    }

    pcb_ProcessChildrenArray_push_to_end(arr, el);
    return res_OK;
}

PCB *pcb_ProcessChildrenArray_remove(pcb_ProcessChildrenArray *arr, size_t index)
{
    assert(index < arr->length);
    assert(arr->arr[index].is_used);
    PCB *pcb = arr->arr[index].pcb;

    arr->arr[index].pcb = NULL; // Set index, has_index and is_used to 0
    if (pcb_ProcessChildrenArray_have_free_elements(arr))
    {
        arr->arr[index].has_index = 1;
        arr->arr[index].index = arr->last_free_idx;
        arr->last_free_idx = index;
    }

    return pcb;
}

size_t pcb_ProcessChildrenArray_find(pcb_ProcessChildrenArray *arr, uint64_t pid)
{
    for (size_t i = 0; i < arr->length; i++)
    {
        if (arr->arr[i].is_used && arr->arr[i].pcb->id == pid)
        {
            assert(arr->arr[i].is_used);
            return i;
        }
    }

    return arr->length;
}
