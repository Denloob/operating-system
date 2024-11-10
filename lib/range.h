#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint64_t begin;
    uint64_t size;
} range_Range ;

/**
 * @brief Sorts and consolidates all elements in a range.
 *
 * @param range_arr Pointer to an array of ranges of length `length`
 * @param length Length of the array
 *
 * @return The new length of the array (No reallocations are done,
 *          the length is logical only)
 */
uint64_t range_defragment(range_Range *range_arr, uint64_t length);

/**
 * @brief Compares two ranges based on their `begin`.
 *
 * @return Positive value if `range` is larger, negative if `other` is
 *          larger, zero if both are equal.
 */
int range_cmp(range_Range *range, range_Range *other);

/**
 * @brief - Removes all ranges with size 0 from range_arr.
 *
 * @param range_arr - Pointer to an array of ranges of length `length`
 * @param length - Length of the array
 *
 * @return The new length of range_arr.
 */
uint64_t range_remove_empty(range_Range *range_arr, uint64_t length);

/**
 * @brief - Finds and removes a subrange of specified size from an array of ranges.
 *
 * Searches through `range_arr` for a range that is at least `target_range_size`
 * bytes large. If found, it reduces that range by `target_range_size` and
 * outputs the starting address of the removed subrange in `addr_out`.
 *
 * If the subrange is the size, of the actual range element, the total size of
 * that range will become 0. The length of the array is not modified.
 *
 * For better results, defragment the array first.
 * @see - range_defragment
 * 
 * @param range_arr         - Pointer to an array of ranges of length `length`
 * @param length            - Length of the array
 * @param target_range_size - The minimum size of the subrange to pop from a range in `range_arr`.
 * @param addr_out[out]     - Pointer to store the beginning address of the removed subrange.
 * 
 * @return `true` on success, false on failure.
 */
bool range_pop_of_size(range_Range *range_arr, uint64_t length, uint64_t target_range_size, uint64_t *addr_out);

/**
 * @brief - Finds a remove a subrange of the specified size or less from an array of ranges.
 *
 * @see - range_pop_of_size
 *
 * @param max_range_size - The maximum range size to pop. The popped range can be
 *                          either of this size, or less.
 * @param addr_out[out]  - Pointer to store the beginning address of the removed subrange.
 * @param size_out[out]  - Pointer to store the actual size of the popped range.
 *                          If NULL, nothing is written into it.
 * @return `true` on success, false on failure
 */
bool range_pop_of_size_or_less(range_Range *range_arr, uint64_t length, uint64_t max_range_size, uint64_t *addr_out, uint64_t *size_out);
