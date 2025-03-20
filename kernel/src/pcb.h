#pragma once

#include "file_descriptor_hashmap.h"
#include "mmu.h"
#include "fs.h"
#include "regs.h"
#include "res.h"
#include <stdint.h>

typedef enum
{
    PCB_STATE_TERMINATED,
    PCB_STATE_RUNNING,
    PCB_STATE_WAITING_FOR_IO,
    PCB_STATE_READY,
    PCB_STATE_ZOMBIE,
} pcb_State;

typedef enum 
{
    WINDOW_TEXT,
    WINDOW_GRAPHICS
} WindowMode;

typedef struct 
{
    WindowMode mode;
    void *buffer; 
    size_t width, height;
} Window;

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

#define res_pcb_ProcessChildrenArray_OUT_OF_MEMORY "Ran out of memory"

typedef struct {
    size_t length;
    size_t capacity;
    size_t last_free_idx;
    union {
        PCB *pcb;
        struct {
            uint64_t index : 62;
            uint64_t has_index : 1;
            uint64_t is_used : 1; // PCB addr highest address is always set
        };
    } *arr;
} pcb_ProcessChildrenArray;

bool pcb_ProcessChildrenArray_init(pcb_ProcessChildrenArray *arr);
PCB *pcb_ProcessChildrenArray_remove(pcb_ProcessChildrenArray *arr, size_t index);
res pcb_ProcessChildrenArray_push(pcb_ProcessChildrenArray *arr, PCB *el);
void pcb_ProcessChildrenArray_cleanup(pcb_ProcessChildrenArray *arr);
// Returns the index of the PCB with `pid` if found. Otherwise arr->length;
size_t pcb_ProcessChildrenArray_find(pcb_ProcessChildrenArray *arr, uint64_t pid);

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
    int         return_code; // Accessible only if state is PCB_STATE_ZOMBIE

    char cwd[FS_MAX_FILEPATH_LEN];
    pcb_IORefresh refresh; // Used only in IO doubly-linked list
    void *refresh_arg;

    mmu_PageMapEntry *paging;

    pcb_ProcessChildrenArray children;

    FileDescriptorHashmap fd_map;
    uint64_t last_fd;

    Window *window;
    // TODO: signal info
};

void PCB_cleanup(PCB *pcb);
PCB* PCB_init(uint64_t id, PCB *parent, uint64_t entry_point, mmu_PageMapEntry *kernel_pml);

int create_window_for_process(PCB *process , WindowMode mode);
