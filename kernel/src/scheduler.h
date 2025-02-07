#pragma once

#include "isr.h"
#include "pcb.h"

#define SCHEDULER_NOT_A_PIC_INTERRUPT -1

/**
 * @brief - Performs a context switch into the process of the given PCB.
 *
 * @param pcb - The PCB of the process to context switch to.
 * @param pic_number - If the request comes from a PIC interrupt, must be set to the
 *                          number of the PIC interrupt. Otherwise, set to SCHEDULER_NOT_A_PIC_INTERRUPT.
 */
void scheduler_context_switch_to(PCB *pcb, int pic_number) __attribute__((noreturn));

/**
 * @brief - Performs a context switch from the currently running process, with the
 *            registers `regs` and `frame` (which contain the RIP and RSP).
 *          Will context switch to the next process in the process queue.
 *
 * @param regs  - The regs and the SSE state of the process (excluding $rsp and $rip)
 * @param frame - Interrupt frame of the process. Used to get its $rsp and $rip.
 * @param pic_number - If the request comes from a PIC interrupt, must be set to the
 *                          number of the PIC interrupt. Otherwise, set to SCHEDULER_NOT_A_PIC_INTERRUPT.
 */
void scheduler_context_switch_from(Regs *regs, isr_InterruptFrame *frame, int pic_number) __attribute__((used, sysv_abi)) __attribute__((noreturn));

/**
 * @brief - Adds the process described by the PCB to the process queue.
 *
 * @param pcb - The PCB of the process to enqueue.
 */
void scheduler_process_enqueue(PCB *pcb);

/**
 * @brief Starts scheduling processes in the process queue. Exactly one process must be in the queue.
 *          Enables the scheduler. Doesn't return.
 *
 * @see scheduler_process_enqueue
 * @see scheduler_enable
 */
void scheduler_start() __attribute__((noreturn));

/**
 * @brief Get the PCB of the current running process.
 */
PCB *scheduler_current_pcb();

/**
 * @brief Dequeue the current process from the queue and context switch to the next one in the queue.
 */
void scheduler_process_dequeue_current_and_context_switch() __attribute__((noreturn));

void scheduler_enable();
void scheduler_disable();

/**
 * @brief - Push to the global IO list.
 *
 * @param pcb - The pcb to remove
 * @param refresh_func - The function to refresh the PCB with.
 * @see - pcb_IORefresh
 */
void scheduler_io_push(PCB *pcb, pcb_IORefresh refresh_func);

/**
 * @brief - Remove from the global IO list
 *
 * @param pcb - The pcb to remove
 */
void scheduler_io_remove(PCB *pcb);

/**
 * @brief Refresh the IO list by going over each element and checking if the IO
 *          operation is complete. If it is, the PCB is rescheduled to the process
 *          queue.
 *
 * @return pointer to the first PCB rescheduled, if any. NULL otherwise.
 */
PCB *scheduler_io_refresh();

void scheduler_move_current_process_to_io_queue_and_context_switch(pcb_IORefresh refresh_func) __attribute__((noreturn));

/**
 * @brief Find a process with the given pid.
 * @return PCB of that process if found, NULL otherwise.
 */
PCB *scheduler_find_by_pid(uint64_t pid);
