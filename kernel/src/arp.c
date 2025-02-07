#include "arp.h"
#include "compiler_macros.h"
#include "hashmap_utils.h"
#include "assert.h"
#include "ipv4.h"
#include "net.h"
#include "rtl8139.h"
#include "smartptr.h"
#include "memory.h"
#include "kmalloc.h"
#include "ethernet.h"
#include "ipv4_constants.h"
#include "mutex.h"
#include "time.h"

typedef struct {
    uint8_t ip[IPv4_IP_ADDR_SIZE]; // The hashmap key
    uint8_t mac[ETHER_MAC_SIZE];
} ArpCacheEntry;

static struct {
    ArpCacheEntry *buf;
    size_t capacity;
    Mutex mutex;
    bool is_initialized; // false by default (global var => zero initialized)
} g_arp_cache;

// NOTE: g_arp_cache mutex must be owned by the caller!
static bool arp_cache_init()
{
    assert(!g_arp_cache.is_initialized);

#define STARTING_CAPACITY 16
    g_arp_cache.capacity = STARTING_CAPACITY;
    g_arp_cache.buf = kcalloc(g_arp_cache.capacity, sizeof(*g_arp_cache.buf));
    if (g_arp_cache.buf == NULL)
    {
        g_arp_cache.capacity = 0;
        return false;
    }

    g_arp_cache.is_initialized = true;
    return true;
}

__attribute__((pure))
static bool is_ip_all_zero(const uint8_t ip[static IPv4_IP_ADDR_SIZE])
{
    for (int i = 0; i < IPv4_IP_ADDR_SIZE; i++)
    {
        if (ip[i] != 0)
        {
            return false;
        }
    }

    return true;
}

__attribute__((pure))
static bool is_arp_cache_emtpy(const ArpCacheEntry *entry)
{
    return is_ip_all_zero(entry->ip);
}

static uint8_t *arp_cache_emplace(const uint8_t ip[static IPv4_IP_ADDR_SIZE]);

// NOTE: g_arp_cache mutex must be owned by the caller!
static bool arp_cache_resize()
{
    assert(g_arp_cache.capacity != 0 && "How did we get here?");
    size_t new_capacity = g_arp_cache.capacity * 2;
    const size_t old_capacity = g_arp_cache.capacity;
    typeof(g_arp_cache.buf) new_buf = kcalloc(new_capacity, sizeof(*new_buf));
    if (new_buf == NULL)
    {
        return false;
    }

    typeof(g_arp_cache.buf) old_buf = g_arp_cache.buf;
    g_arp_cache.buf = new_buf;
    g_arp_cache.capacity = new_capacity;

    for (size_t i = 0; i < old_capacity; i++)
    {
        if (!is_arp_cache_emtpy(&old_buf[i]))
        {
            uint8_t *mac = arp_cache_emplace(old_buf[i].ip);
            if (mac == NULL)
            {
                g_arp_cache.capacity = old_capacity;
                g_arp_cache.buf = old_buf;
                kfree(new_buf);
                return false;
            }

            memmove(mac, old_buf[i].mac, sizeof(old_buf[i].mac));
        }
    }

    kfree(old_buf);

    return true;
}

__attribute__((pure))
static uint32_t ip_to_u32(const uint8_t ip[static IPv4_IP_ADDR_SIZE])
{
    uint32_t ip_u32;
    _Static_assert(sizeof(ip_u32) == IPv4_IP_ADDR_SIZE, "IPv4 is the size of a uint32_t");
    memmove(&ip_u32, ip, IPv4_IP_ADDR_SIZE);

    return ip_u32;
}

// NOTE: g_arp_cache mutex must be owned by the caller!
static uint8_t *arp_cache_emplace(const uint8_t ip[static IPv4_IP_ADDR_SIZE])
{
    if (is_ip_all_zero(ip))
    {
        return NULL;
    }

    if (!g_arp_cache.is_initialized)
    {
        arp_cache_init();
    }
#define HASH_LOOP_HARD_LIMIT 50
    int iteration_counter = 0;

    const uint32_t ip_u32 = ip_to_u32(ip);

    uint64_t hash_state = ip_u32;
    size_t index;
    do
    {
        iteration_counter++;
        if (iteration_counter > HASH_LOOP_HARD_LIMIT)
        {
            bool success = arp_cache_resize();
            if (!success)
            {
                return NULL;
            }

            iteration_counter = 0;
            hash_state = ip_u32;
        }
        hash_state = hash_u64(hash_state);
        index = hash_state % g_arp_cache.capacity;
    }
    while (!is_arp_cache_emtpy(&g_arp_cache.buf[index]));

    _Static_assert(sizeof(g_arp_cache.buf[index].ip) == IPv4_IP_ADDR_SIZE, "IPv4 in the cache should the size of an IP address");
    memmove(g_arp_cache.buf[index].ip, ip, sizeof(g_arp_cache.buf[index].ip));

    return g_arp_cache.buf[index].mac;
}

bool arp_cache_query(const uint8_t ip[static IPv4_IP_ADDR_SIZE], uint8_t out_mac[static ETHER_MAC_SIZE])
{
    if (is_ip_all_zero(ip))
    {
        return false;
    }

    mutex_lock(&g_arp_cache.mutex);
    defer({ mutex_unlock(&g_arp_cache.mutex); });

    if (!g_arp_cache.is_initialized)
    {
        arp_cache_init();
    }

    int iteration_counter = 0;

    const uint32_t ip_u32 = ip_to_u32(ip);

    uint64_t hash_state = ip_u32;
    size_t index;

    do
    {
        hash_state = hash_u64(hash_state);

        index = hash_state % g_arp_cache.capacity;

        if (memcmp(g_arp_cache.buf[index].ip, ip, IPv4_IP_ADDR_SIZE) == 0)
        {
            memmove(out_mac, g_arp_cache.buf[index].mac, ETHER_MAC_SIZE);
            return true;
        }

        iteration_counter++;
    }
    while (iteration_counter <= HASH_LOOP_HARD_LIMIT);

    return false;
}

// Initialize basic stuff like hardware_type, sender mac/ip, etc
// @return - true on success. Returns false when we don't have an IP yet
// @see - ipv4_get_own_ip
WUR
static bool arp_packet_init_constants(arp_IPv4EtherPacket *arp)
{
    bool success = ipv4_get_own_ip(arp->sender_ip);
    if (!success)
    {
        return false;
    }

    arp->hardware_type      = htons(arp_HARDWARE_TYPE_ETHER);
    arp->protocol_type      = htons(arp_PROTOCOL_TYPE_IPv4);
    arp->hardware_length    = ETHER_MAC_SIZE;
    arp->protocol_length    = IPv4_IP_ADDR_SIZE;

    rtl8139_get_mac_address(arp->sender_mac);
    return true;
}

static void arp_send_reply(const arp_IPv4EtherPacket *request)
{
    char buf[ETHER_PACKET_SIZE + sizeof(arp_IPv4EtherPacket)];

    // Ethernet packet

    EthernetPacket *ether = (EthernetPacket *)buf;

    memset(ether->dest_mac, 0xff, sizeof(ether->dest_mac));
    rtl8139_get_mac_address(ether->source_mac);
    ether->ethertype = htons(ethernet_TYPE_ARP);

    // ARP packet

    arp_IPv4EtherPacket *arp = (arp_IPv4EtherPacket *)ether->data_and_fcs;
    bool success = arp_packet_init_constants(arp);
    if (!success)
    {
        return; /* DROP */
    }

    arp->operation = htons(arp_OPERATION_REPLY);

    memmove(arp->target_ip, request->sender_ip, sizeof(arp->target_ip));
    memmove(arp->target_mac, request->sender_mac, sizeof(arp->target_mac));

    while (!rtl8139_try_transmit_packet(ether, sizeof(buf) - ETHER_PACKET_SIZE))
    {
    }
}

static void arp_send_request(const uint8_t ip[static IPv4_IP_ADDR_SIZE])
{
    char buf[ETHER_PACKET_SIZE + sizeof(arp_IPv4EtherPacket)];

    // Ethernet packet

    EthernetPacket *ether = (EthernetPacket *)buf;

    memset(ether->dest_mac, 0xff, sizeof(ether->dest_mac));
    rtl8139_get_mac_address(ether->source_mac);
    ether->ethertype = htons(ethernet_TYPE_ARP);

    // ARP packet

    arp_IPv4EtherPacket *arp = (arp_IPv4EtherPacket *)ether->data_and_fcs;
    bool success = arp_packet_init_constants(arp);
    if (!success)
    {
        return; /* DROP */
    }

    arp->operation = htons(arp_OPERATION_REQUEST);

    memmove(arp->target_ip, ip, sizeof(arp->target_ip));
    memset(arp->target_mac, 0, sizeof(arp->target_mac));

    while (!rtl8139_try_transmit_packet(ether, sizeof(buf) - ETHER_PACKET_SIZE))
    {
    }
}

static void arp_resolve_via_request(const uint8_t ip[static IPv4_IP_ADDR_SIZE], uint8_t out_mac[static ETHER_MAC_SIZE])
{
    arp_send_request(ip);

    // When the response arrives, it will be added to the cache without us.
    //  Hence, all we need to do, is to wait for it to appear in the cache.

    while (true)
    {
        bool found = arp_cache_query(ip, out_mac);
        if (found)
        {
            return;
        }

        sleep_ms(1);
    }
}

void arp_resolve_ip_to_mac(const uint8_t ip[static IPv4_IP_ADDR_SIZE], uint8_t out_mac[static ETHER_MAC_SIZE])
{
    bool found = arp_cache_query(ip, out_mac);
    if (found)
    {
        return;
    }

    arp_resolve_via_request(ip, out_mac);
}

void arp_handle(EthernetPacket *packet, int data_length)
{
    const arp_IPv4EtherPacket *arp = (arp_IPv4EtherPacket *)packet->data_and_fcs;
    if (data_length < sizeof(*arp))
    {
        return; /* DROP */
    }

    bool is_ipv4_ether_packet =
        ntohs(arp->hardware_type) == arp_HARDWARE_TYPE_ETHER &&
        ntohs(arp->protocol_type) == arp_PROTOCOL_TYPE_IPv4 &&
        arp->hardware_length == ETHER_MAC_SIZE &&
        arp->protocol_length == IPv4_IP_ADDR_SIZE;
    if (!is_ipv4_ether_packet)
    {
        return; /* DROP */
    }

    switch (ntohs(arp->operation))
    {
        case arp_OPERATION_REPLY:
        {
            mutex_lock(&g_arp_cache.mutex);
            defer({ mutex_unlock(&g_arp_cache.mutex); });

            uint8_t *mac = arp_cache_emplace(arp->sender_ip);
            if (mac == NULL)
            {
                // Out of memory or invalid IP, drop
                return;
            }

            memmove(mac, arp->sender_mac, ETHER_MAC_SIZE);

            break;
        }
        case arp_OPERATION_REQUEST:
            arp_send_reply(arp);
            break;
    }
}
