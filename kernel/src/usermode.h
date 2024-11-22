#pragma once

#define USERMODE_PROCESS_BASE (void *)0x400000
#define USERMODE_STACK_BASE   (void *)0xc0000000

/**
 * @brief Switches ring 3 (usermode) and jumps to the given address, with
 *          rsp and rbp being the given `stack` address.
 *
 * @param address - The usermode code address to jump to.
 * @param stack   - The base of the usermode stack.
 */
void usermode_jump_to(void *address, void *stack) __attribute__((noreturn));
