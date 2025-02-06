#include "net_callback_registry.h"
#include "hashmap_utils.h"
#include "kmalloc.h"
#include "assert.h"
#include "memory.h"

static NetCallbackRegistryEntry *net_callback_registry_emplace(NetCallbackRegistry *registry, NetCallbackRegistryKey key);

__attribute__((const))
static uint64_t key_to_u64(NetCallbackRegistryKey key)
{
    union {
        uint64_t int_key;
        NetCallbackRegistryKey struct_key;
    } type_punned_key = { .struct_key = key };

    _Static_assert(sizeof(key) == sizeof(type_punned_key.int_key), "NetCallbackRegistryKey must be 64bit in size");

    return type_punned_key.int_key;
}

__attribute__((const))
static uint64_t hash(NetCallbackRegistryKey key)
{
    return hash_u64(key_to_u64(key));
}

bool net_callback_registry_init(NetCallbackRegistry *registry)
{
#define STARTING_CAPACITY 16
    registry->capacity = STARTING_CAPACITY;
    registry->buf = kcalloc(registry->capacity, sizeof(*registry->buf));
    if (registry->buf == NULL)
    {
        registry->capacity = 0;
        return false;
    }

    return true;
}

__attribute__((pure))
static bool net_callback_registry_entry_is_in_use(NetCallbackRegistryEntry *entry)
{
    return entry->callback != NULL;
}

static bool net_callback_registry_resize(NetCallbackRegistry *registry)
{
    assert(registry->capacity != 0 && "How did we get here?");
    size_t new_capacity = registry->capacity * 2;
    const size_t old_capacity = registry->capacity;
    typeof(registry->buf) new_buf = kcalloc(new_capacity, sizeof(*new_buf));
    if (new_buf == NULL)
    {
        return false;
    }

    typeof(registry->buf) old_buf = registry->buf;
    registry->buf = new_buf;
    registry->capacity = new_capacity;

    for (size_t i = 0; i < old_capacity; i++)
    {
        if (net_callback_registry_entry_is_in_use(&old_buf[i]))
        {
            NetCallbackRegistryEntry *desc = net_callback_registry_emplace(registry, old_buf[i].key);
            if (desc == NULL)
            {
                registry->capacity = old_capacity;
                registry->buf = old_buf;
                kfree(new_buf);
                return false;
            }

            *desc = old_buf[i];
        }
    }

    kfree(old_buf);

    return true;
}

static NetCallbackRegistryEntry *net_callback_registry_emplace(NetCallbackRegistry *registry, NetCallbackRegistryKey key)
{
    assert(hash(key) != 0);
#define HASH_LOOP_HARD_LIMIT 50
    int iteration_counter = 0;

    const uint64_t initial_hash_state = hash(key);
    uint64_t hash_state = initial_hash_state;
    size_t index;
    do
    {
        iteration_counter++;
        if (iteration_counter > HASH_LOOP_HARD_LIMIT)
        {
            bool success = net_callback_registry_resize(registry);
            if (!success)
            {
                return NULL;
            }

            iteration_counter = 0;
            hash_state = initial_hash_state;
        }
        hash_state = hash_u64(hash_state);
        index = hash_state % registry->capacity;
    }
    while (net_callback_registry_entry_is_in_use(&registry->buf[index]));

    registry->buf[index].key = key;
    return &registry->buf[index];
}

static NetCallbackRegistryEntry *net_callback_registry_get(NetCallbackRegistry *registry, NetCallbackRegistryKey key)
{
    int iteration_counter = 0;
    uint64_t hash_state = hash(key);
    size_t index;
    do
    {
        hash_state = hash_u64(hash_state);

        index = hash_state % registry->capacity;

        NetCallbackRegistryEntry *entry = &registry->buf[index];
        if (key_to_u64(entry->key) == key_to_u64(key))
        {
            return entry;
        }

        iteration_counter++;
    }
    while (iteration_counter <= HASH_LOOP_HARD_LIMIT);

    return NULL;
}

bool net_callback_registry_handle_packet(NetCallbackRegistry *registry, NetCallbackRegistryKey key, void *packet)
{
    NetCallbackRegistryEntry *entry = net_callback_registry_get(registry, key);
    if (entry == NULL)
    {
        return false;
    }

    entry->callback(entry->state, packet);

    entry->callback = NULL; // Remove the entry from the registry.
    return true;
}

bool net_callback_registry_register(NetCallbackRegistry *registry, NetCallbackRegistryKey key, NetCallback callback, void *state)
{
    assert(net_callback_registry_get(registry, key) == NULL);

    NetCallbackRegistryEntry *entry = net_callback_registry_emplace(registry, key);
    if (entry == NULL)
    {
        return false;
    }

    entry->callback = callback;
    entry->state = state;

    return true;
}
