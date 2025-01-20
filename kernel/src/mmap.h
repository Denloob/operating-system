#pragma once

#include <stddef.h>
#include <stdint.h>
#include "compiler_macros.h"
#include "range.h"
#include "res.h"

typedef enum {
    MMAP_PROT_NONE  = 0x0,
    MMAP_PROT_READ  = 0x1,
    MMAP_PROT_WRITE = 0x2,
    MMAP_PROT_EXEC  = 0x4,
    MMAP_PROT_RING_3= 0x8,
} mmap_Protection;

#define res_mmap_OUT_OF_MEMORY    "cannot allocate memory"
#define res_mmap_INVALID_SIZE     "size cannot be 0"
#define res_mmap_MUST_BE_READABLE "non readable prot is not supported"

/**
 * @brief Initialize the mmap memory map
 *
 * @param mmap_base The memory map base
 * @param length The length of the memory map
 * @note  The memory map was most likely passed to the kernel by the bootloader.
 */
void mmap_init(range_Range *mmap_base, uint64_t length);

/**
 * @brief Map a memory region starting from address of size `size` with `prot`
 *          protections.
 *
 * @param addr The (page aligned) beginning of the (virtual) memory region.
 * @param size The size of the region.
 * @param prot The protections for the region.
 * @return res_OK or one of the errors defined above.
 */
res mmap(void *addr, size_t size, mmap_Protection prot) WUR;

/**
 * @brief Unmap memory pages.
 * @note  Ideally, only call it on addresses and sizes from mmap.
 *          However,
 *          it is ok to use this to unmap *page aligned* addresses, with
 *          *page aligned* size, even if it does not come from mmap
 *          when it's actual physical ram memory. Do this only if you know what
 *          you are doing.
 *
 * @param addr The (page aligned) address to unmap.
 * @param size The size of the region to unmap.
 */
void munmap(void *addr, size_t size);

/**
 * @brief Change the protection flags on pages.
 *
 * @param addr The (page aligned) beginning of the (virtual) memory region.
 * @param size The size of the region.
 * @param prot The new protections for the region.
 * @return res_OK or one of the errors defined above.
 */
res mprotect(void *addr, size_t size, mmap_Protection prot);

/**
 * @brief - Allocate contiguous physical memory of size `wanted_size`.
 *
 * @param want_size The size of the contiguous memory to allocate.
 *                      Must be page aligned
 * @param out_result[out]      - The start address of the allocated physical memory.
 * @return - true on success, false otherwise. If the function didn't succeed
 *              there's no free contiguous memory range of the requested size.
 */
bool mmap_allocate_contiguous(uint64_t want_size, uint64_t *out_result);
