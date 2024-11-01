#pragma once

#include <stdint.h>

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
