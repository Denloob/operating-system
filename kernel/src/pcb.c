#include "pcb.h"
#include "mmap.h"
#include "file_descriptor_hashmap.h"
#include "mmu.h"
#include "mmu_config.h"
#include "assert.h"
#include "kmalloc.h"
#include "memory.h"
#include "res.h"
#include <stdint.h>

const static int kernel_start_index = 256;
PCB* PCB_init(uint64_t id, PCB *parent, uint64_t entry_point, mmu_PageMapEntry *kernel_pml)
{
    const int kernel_end_index = 512;

    PCB* created_pcb = kcalloc(1, sizeof(struct PCB));
    if (created_pcb == NULL)
    {
        return NULL;
    }

    if (!file_descriptor_hashmap_init(&created_pcb->fd_map))
    {
        kfree(created_pcb);
        return NULL;
    }

    if (!pcb_ProcessChildrenArray_init(&created_pcb->children))
    {
        file_descriptor_hashmap_cleanup(&created_pcb->fd_map);
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

    mmu_PageMapEntry *paging = pcb->paging;

    for (int level4 = 0; level4 < kernel_start_index; level4++)
    {
        if (paging[level4].present == 0) continue;
        mmu_PageMapEntry *l3 = (void *)mmu_page_table_entry_address_get_virt(&paging[level4]);
        for (int level3 = 0; level3 < TABLE_LENGTH; level3++)
        {
            if (l3[level3].present == 0) continue;
            mmu_PageMapEntry *l2 = (void *)mmu_page_table_entry_address_get_virt(&l3[level3]);
            for (int level2 = 0; level2 < TABLE_LENGTH; level2++)
            {
                if (l2[level2].present == 0) continue;
                mmu_PageTableEntry *l1 = (void *)mmu_page_table_entry_address_get_virt(&l2[level2]);

                for (int level1 = 0; level1 < TABLE_LENGTH; level1++)
                {
                    if (l1[level1].present == 0) continue;
                    mmap_phys_memory_add(&(range_Range){
                        .begin = mmu_page_table_entry_address_get(&l1[level1]),
                        .size = PAGE_SIZE,
                    });
                }

                mmu_map_deallocate(l1);
            }
            mmu_map_deallocate(l2);
        }
        mmu_map_deallocate(l3);
    }

    mmu_map_deallocate(paging);

    for (size_t i = 0; i < pcb->children.length; i++)
    {
        if (!pcb->children.arr[i].is_used)
        {
            continue;
        }

        PCB *child_pcb = pcb->children.arr[i].pcb;
        child_pcb->parent = NULL;
        if (child_pcb->state == PCB_STATE_ZOMBIE)
        {
            PCB_cleanup(child_pcb);
        }
    }

    file_descriptor_hashmap_cleanup(&pcb->fd_map);
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


int create_window_for_process(PCB *process , WindowMode mode)
{
    if (process->window) return -1; //Will need to change this if we will want to support multiple windows at some point

    Window *win = kmalloc(sizeof(Window));
    if (!win) return -1;

    win->mode = mode;
    #define VGA_TEXT_WINDOW_SIZE 80*25*2
    #define VGA_GRAPGHICS_WINDWOS_SIZE 320*200*3
    if(mode == WINDOW_TEXT)
    {
        win->buffer = kmalloc(VGA_TEXT_WINDOW_SIZE);
    }
    if(mode == WINDOW_GRAPHICS)
    {
        win->buffer = kmalloc(VGA_GRAPGHICS_WINDWOS_SIZE);
    }
    process->window = win;

    return 0;
}
