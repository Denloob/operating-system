#include "assert.h"
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

    assert(new_pivot_idx - 1 <= last_idx);
    quicksort_recursive(arr, first_idx, new_pivot_idx - 1);

    assert(new_pivot_idx + 1 >= first_idx);
    quicksort_recursive(arr, new_pivot_idx + 1, last_idx);
}

static void quicksort(range_Range *arr, uint64_t length)
{
    quicksort_recursive(arr, 0, length - 1);
}

uint64_t range_defragment(range_Range *range_arr, uint64_t length)
{
    quicksort(range_arr, length);

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
