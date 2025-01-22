#include "file_descriptor_hashmap.h"
#include "file_descriptor.h"
#include "kmalloc.h"
#include "memory.h"
#include "assert.h"

__attribute__((const))
static uint64_t hash(uint64_t val)
{
    // Based on https://github.com/h2database/h2database/blob/cc0e45355d78c3ee6711f6bb73eba4f224c4ae18/h2/src/test/org/h2/test/store/CalculateHashConstantLong.java#L75
    val = (val ^ (val >> 30)) * 0xbf58476d1ce4e5b9;
    val = (val ^ (val >> 27)) * 0x94d049bb133111eb;
    val = val ^ (val >> 31);
    return val;
}

bool file_descriptor_hashmap_init(FileDescriptorHashmap *map)
{
#define STARTING_CAPACITY 16
    map->capacity = STARTING_CAPACITY;
    map->buf = kcalloc(map->capacity, sizeof(*map->buf));
    if (map->buf == NULL)
    {
        map->capacity = 0;
        return false;
    }

    return true;
}

static bool file_descriptor_hashmap_resize(FileDescriptorHashmap *map)
{
    assert(map->capacity != 0 && "How did we get here?");
    size_t new_capacity = map->capacity * 2;
    const size_t old_capacity = map->capacity;
    typeof(map->buf) new_buf = kcalloc(new_capacity, sizeof(*new_buf));
    if (new_buf == NULL)
    {
        return false;
    }

    typeof(map->buf) old_buf = map->buf;
    map->buf = new_buf;
    map->capacity = new_capacity;

    for (size_t i = 0; i < old_capacity; i++)
    {
        if (old_buf[i].is_used)
        {
            FileDescriptor *desc = file_descriptor_hashmap_emplace(map, old_buf[i].fd.num);
            if (desc == NULL)
            {
                map->capacity = old_capacity;
                map->buf = old_buf;
                kfree(new_buf);
                return false;
            }

            *desc = old_buf[i].fd;
        }
    }

    kfree(old_buf);

    return true;
}

FileDescriptor *file_descriptor_hashmap_emplace(FileDescriptorHashmap *map, uint64_t fd)
{
    assert(fd != 0);
#define HASH_LOOP_HARD_LIMIT 50
    int iteration_counter = 0;

    uint64_t hash_state = fd;
    size_t index;
    do
    {
        iteration_counter++;
        if (iteration_counter > HASH_LOOP_HARD_LIMIT)
        {
            bool success = file_descriptor_hashmap_resize(map);
            if (!success)
            {
                return NULL;
            }

            iteration_counter = 0;
            hash_state = fd;
        }
        hash_state = hash(hash_state);
        index = hash_state % map->capacity;
    }
    while (map->buf[index].is_used);

    map->buf[index].is_used = true;

    map->buf[index].fd.num = fd;
    return &map->buf[index].fd;
}

FileDescriptor *file_descriptor_hashmap_get(FileDescriptorHashmap *map, uint64_t fd)
{
    int iteration_counter = 0;
    uint64_t hash_state = fd;
    size_t index;
    do
    {
        hash_state = hash(hash_state);

        index = hash_state % map->capacity;

        if (map->buf[index].is_used && map->buf[index].fd.num == fd)
        {
            return &map->buf[index].fd;
        }

        iteration_counter++;
    }
    while (iteration_counter <= HASH_LOOP_HARD_LIMIT);

    return NULL;
}

bool file_descriptor_hashmap_remove(FileDescriptorHashmap *map, uint64_t fd)
{
    int iteration_counter = 0;
    uint64_t hash_state = fd;
    size_t index;
    do
    {
        hash_state = hash(hash_state);

        index = hash_state % map->capacity;

        if (map->buf[index].is_used && map->buf[index].fd.num == fd)
        {
            map->buf[index].is_used = false;
            FileDescriptor *desc = &map->buf[index].fd;
            memset(desc, 0, sizeof(*desc)); // Better safe than sorry
            return true;
        }

        iteration_counter++;
    }
    while (iteration_counter <= HASH_LOOP_HARD_LIMIT);

    return false;
}

void file_descriptor_hashmap_cleanup(FileDescriptorHashmap *map)
{
    kfree(map->buf);
    map->buf = NULL;
    map->capacity = 0;
}
