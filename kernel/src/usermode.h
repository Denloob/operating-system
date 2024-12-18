#pragma once

#include "compiler_macros.h"
#include "regs.h"
#include "res.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define USERMODE_PROCESS_BASE (void *)0x400000
#define USERMODE_STACK_BASE   (void *)0xc0000000

typedef struct usermode_mem usermode_mem;

/**
 * @brief Switches ring 3 (usermode) and jumps to the given address, the registers
 *          specified in `regs`. (Stack address is regs->rsp).
 *
 * @param address - The usermode code address to jump to.
 * @param regs    - The registers to load
 */
void usermode_jump_to(void *address, const Regs *regs) __attribute__((noreturn));

/**
 * @brief Check if the given address is a usermode address.
 *          Either result doesn't mean if it's valid, but it does guarantee
 *          that reading/writing to/from that address will/won't touch usermode/kernel.
 *
 * @param address The address to check.
 * @param size The size of the object at the address. (Important as address can only partially touch kernel).
 * @return True if doesn't touch anything kernel related. False otherwise.
 */
bool is_usermode_address(void *address, size_t size) WUR __attribute__((pure));

void usermode_init_address_check(uint64_t mmu_map_base_address, uint64_t mmu_map_size);

/**
 * @brief - Initialize supported Supervisor Memory Protections (SMEP and/or SMAP), if available.
 */
void usermode_init_smp();

#define res_usermode_NOT_USERMODE_ADDRESS "Not a usermode address"

/**
 * @brief - Copies `size` bytes from usermode to kernel space.
 *
 * @param to   - Destination address, in the kernel space.
 * @param from - Source address, in the user space.
 * @param len  - The length in bytes of the region to copy.
 *
 * @return - res_OK on success, or one of the error codes on failure.
 */
res usermode_copy_from_user(void *to, const usermode_mem *from, size_t len);

/**
 * @brief - Check that the given memory range is mapped to be in usermode.
 */
bool usermode_is_mapped(uint64_t begin, uint64_t end);
