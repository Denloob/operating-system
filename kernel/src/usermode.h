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
 *          Either result doesn't mean if it's valid, but `true` returned guarantees
 *          that reading/writing to/from that address won't touch the kernel and
 *          can actually be mapped (not in the memory hole).
 *
 * @note - If you intend to access that memory, use usermode_is_mapped
 * @see  - usermode_is_mapped
 *
 * @param address The address to check.
 * @param size The size of the object at the address. (Important as address can only partially touch kernel).
 * @return True if doesn't touch anything kernel related. False otherwise.
 */
bool is_usermode_address(const void *address, size_t size) WUR __attribute__((pure));

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
res usermode_copy_from_user(void *to, const usermode_mem *from, size_t len) WUR;

// @see usermode_copy_from_user
res usermode_copy_to_user(usermode_mem *to, const void *from, size_t len) WUR;

/**
 * @brief - Check that the given memory range is mapped to be in usermode.
 */
bool usermode_is_mapped(uint64_t begin, uint64_t end);

/**
 * @brief - Get the length of a usermode, null terminated, string in `str`.
 *          If no null byte is found until max_length is reached,
 *              or until the end of usermode memory, false is returned
 *              and out_len will be set to the amount of bytes read so far.
 *          Otherwise, out_len will contain the length of the string.
 *
 * @param str        - The usermode null terminated string to operate on.
 * @param max_length - The maximum length to check to.For example, `0` would guarantee
 *                      that false is returned. `1` guarantees that true is returned
 *                      iff str points to an existing, usermode, null byte.
 *                      Pass UINT64_MAX to not have a max length.
 * @param out_len[out] - The amount of bytes read until `max_length`, a null byte,
 *                          or the end of usermode mapped memory.
 * @return - true if stopped because of a null byte, false otherwise.
 */
bool usermode_strlen(const usermode_mem *str, uint64_t max_length, uint64_t *out_len);

/**
 * @brief - Get the length of a usermode, null terminated, array in `arr`
 *          where each element is of size `element_size`.
 *          For more details, see usermode_strlen. This function is identical to
 *          usermode_strlen, except that elements of the "string" aren't 1 byte long.
 * @see - usermode_strlen
 *
 * @return - true if stopped because of a null element, false otherwise.
 */
bool usermode_len(const usermode_mem *arr, size_t element_size, uint64_t max_length, uint64_t *out_len);
