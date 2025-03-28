#include "assert.h"
#include "memory.h"
#include "range.h"

// Returns a pointer to the median range
#define MEDIAN(a, b, c) (range_cmp(b,a) < 0)                                      \
              ?   (range_cmp(b,c) < 0)  ?  (range_cmp(c,a) < 0) ? c : a  :  b  \
              :   (range_cmp(a,c) < 0)  ?  (range_cmp(c,b) < 0) ? c : b  :  a;

static void swap(range_Range *first, range_Range *second)
{
    if (first == second) return;
    range_Range tmp = *first;
    *first = *second;
    *second = tmp;
}

uint64_t partition(range_Range *arr, uint64_t first_idx, uint64_t last_idx, range_Range *pivot)
{
    // Make pivot the last element
    swap(pivot, &arr[last_idx]);
    pivot = &arr[last_idx];

    uint64_t i = first_idx;
    uint64_t j = last_idx - 1;
    while (true)
    {
        while (range_cmp(&arr[i], pivot) < 0)
        {
            i++;
        }

        while (j > i && range_cmp(&arr[j], pivot) >= 0)
        {
            j--;
        }

        if (i >= j) break;

        swap(&arr[i], &arr[j]);
        i++;
    }

    swap(pivot, &arr[i]);

    return i;
}


static void quicksort_recursive(range_Range *arr, uint64_t first_idx,
                                uint64_t last_idx)
{
    assert(first_idx <= last_idx);
    uint64_t length = last_idx - first_idx;
    assert(!__builtin_add_overflow(length, 1, &length));

    if (length <= 1)
    {
        return;
    }

    if (length == 2)
    {
        if (range_cmp(&arr[first_idx], &arr[last_idx]) > 0)
            swap(&arr[first_idx], &arr[last_idx]);
        return;
    }

    uint64_t mid_idx = length / 2;
    // Choose our pivot as the median of the first, middle and last ranges.
    range_Range *pivot = MEDIAN(&arr[first_idx], &arr[mid_idx], &arr[last_idx]);

    uint64_t new_pivot_idx = partition(arr, first_idx, last_idx, pivot);

    // `pivot` has been moved by partition into the middle, between smaller and larger
    //  values. All is left is to complete the sort on the sub arrays.

    if (first_idx < new_pivot_idx)
    {
        quicksort_recursive(arr, first_idx, new_pivot_idx - 1);
    }

    if (new_pivot_idx + 1 < last_idx)
    {
        quicksort_recursive(arr, new_pivot_idx + 1, last_idx);
    }
}

static void quicksort(range_Range *arr, uint64_t length)
{
    quicksort_recursive(arr, 0, length - 1);
}

uint64_t range_remove_empty(range_Range *range_arr, uint64_t length)
{
    if (length == 0) return 0;

    uint64_t write_pos = 0;
    for (uint64_t i = 0; i < length; i++)
    {
        if (range_arr[i].size != 0)
        {
            if (write_pos != i)
            {
                range_arr[write_pos] = range_arr[i];
            }
            write_pos++;
        }
    }
    return write_pos;
}

uint64_t merge_adjacent(range_Range *range_arr, uint64_t length)
{
    if (length <= 1) return length;

    uint64_t write_pos = 0;
    range_Range current = range_arr[0];

    for (uint64_t i = 1; i < length; i++)
    {
        uint64_t current_end = current.begin + current.size;
        uint64_t next_begin = range_arr[i].begin;
        uint64_t next_end = range_arr[i].begin + range_arr[i].size;

        if (current_end >= next_begin)
        {
            if (next_end > current_end)
            {
                current.size = next_end - current.begin;
            }
        }
        else
        {
            range_arr[write_pos] = current;
            write_pos++;
            current = range_arr[i];
        }
    }

    range_arr[write_pos] = current;
    write_pos++;

    return write_pos;
}

uint64_t range_defragment(range_Range *range_arr, uint64_t length)
{
    assert(length != 0 && "length cannot be 0");
    quicksort(range_arr, length);

    length = range_remove_empty(range_arr, length);
    length = merge_adjacent(range_arr, length);

    return length;
}

int range_cmp(range_Range *range, range_Range *other)
{
    if (range->begin > other->begin)
        return 1;
    if (range->begin < other->begin)
        return -1;
    return 0;
}

/**
 * @brief - Given a range `target`, and a size `wanted_length`, reduces
 *              the size of the `target` by wanted_length and returns a pointer
 *              to now not-occupied region.
 * @note - `wanted_length` must fit in the `target`. If their size match exactly,
 *           the `target` size will become 0.
 */
uint64_t extract_subrange_from(range_Range *target, uint64_t wanted_length)
{
    assert(target->size >= wanted_length);
    uint64_t ret = target->begin;
    target->begin += wanted_length;
    target->size -= wanted_length;

    return ret;
}

bool range_pop_of_size(range_Range *range_arr, uint64_t length, uint64_t target_range_size, uint64_t *addr_out)
{
    assert((target_range_size & 0xfff) == 0 && "The range length has to be page aligned"); // This is technically not a `range` feature, but if it ever becomes a problem, separate it into two functions.

    for (int i = 0; i < length; i++)
    {
        if (range_arr[i].size >= target_range_size)
        {
             *addr_out = extract_subrange_from(&range_arr[i], target_range_size);
              return true;
        }
    }

    return false;
}

bool range_pop_of_size_or_less(range_Range *range_arr, uint64_t length, uint64_t max_range_size, uint64_t *addr_out, uint64_t *size_out)
{
    assert((max_range_size & 0xfff) == 0 && "The max range length has to be page aligned");

    if (length == 0) return false;

    range_Range *largest_range = NULL;
    for (int i = 0; i < length; i++)
    {
        // The ideal scenario, we return an address of the exact size requested
        if (range_arr[i].size >= max_range_size)
        {
            *addr_out = extract_subrange_from(&range_arr[i], max_range_size);
            if (size_out) *size_out = max_range_size;

            return true;
        }

        if (largest_range == NULL || range_arr[i].size > largest_range->size)
        {
            largest_range = &range_arr[i];
        }
    }

    // No range of the requested size was found, so return the largest possible range.
    *addr_out = largest_range->begin;
    if (size_out) *size_out = largest_range->size;
    largest_range->size = 0;

    return true;
}
