#pragma once

#include "compiler_macros.h"
#include "mmap.h"
#include "res.h"
#include <stdint.h>

#define res_brk_CHANGE_TO_LARGE "The requested BRK change was to big to account for"

/**
 * @brief Move the page break to `addr`. If addr is above current page break,
 *          allocate all the memory in between. If less, deallocate all the
 *          memory in between.
 *
 * @param addr The new address for the page break.
 * @return res_OK on success, or one of the error codes on failure (also includes mmap error codes).
 */
res kbrk(void *addr) WUR;

/**
 * @brief Move the page break. 
 *
 * @param increment - The increment by which to move the page break.
 *          If increment is positive, allocates that many bytes.
 *          If increment is negative, deallocates.
 *          Increment of 0 can be used to get the current page break.
 * @param prev_brk[out] - the brk before the increment. That means that when
 *          increment is 0, it's the current page break. And when increment is
 *          positive, it points to the newly allocated region. Can be NULL.
 *
 * @return res_OK on success, or one of the error codes on failure (also includes mmap error codes).
 */
res ksbrk(int64_t increment, void **prev_brk) WUR;

// These functions are the same as kbrk/ksbrk but act on the given page_break_state
//  instead of a global one, and allow specifying the allocated page protections.
res brk(void *addr_in, uint64_t *page_break_state, mmap_Protection prot);
res sbrk(int64_t increment, void **prev_brk, uint64_t *page_break_state, mmap_Protection prot);
