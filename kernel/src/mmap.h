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
 * @param addr The beginning of the (virtual) memory region.
 * @param size The size of the region.
 * @param prot The protections for the region.
 * @return res_OK or one of the errors defined above.
 */
res mmap(void *addr, size_t size, mmap_Protection prot) WUR;
