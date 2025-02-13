#include "ipv4.h"
#include "io.h"
#include "memory.h"
#include "ipv4_constants.h"
#include "net_callback_registry.h"
#include "UDP.h"
#include "assert.h"

static NetCallbackRegistry g_registry;
bool g_is_registry_initalized = false;

static void initialize_registry()
{
    assert(!g_is_registry_initalized);
    bool success = net_callback_registry_init(&g_registry);
    assert(success);
    g_is_registry_initalized = true;
}

// Returns true if `out` was written to, false otherwise (aka the packet protocol
//  is unsupported).
static bool ipv4_packet_to_key(EthernetPacket *packet, int data_length, NetCallbackRegistryKey *out)
{
    IPv4Packet *ip_packet = (IPv4Packet *)&packet->data_and_fcs[0];
    switch (ip_packet->protocol)
    {
        // TODO: based on the protocol, extract the relevant data and set the key.
        case IPv4_PROTOCOL_ICMP:
            *out = ipv4_create_protocol_key_icmp(ip_packet->source_ip);
            return true;
        case IPv4_PROTOCOL_UDP:
        {
            UDPHeader *udp = (UDPHeader *)ip_packet->option_and_data;
            *out = ipv4_create_protocol_key_udp(ip_packet->source_ip, udp->dest_port);
            return true;
        }
        default:
            /* Unsupported protocol, drop */
            return false;
    }

}

void ipv4_handle(EthernetPacket *packet, int data_length)
{
    if (!g_is_registry_initalized)
    {
        initialize_registry();
    }

    NetCallbackRegistryKey key;
    bool supported_protocol = ipv4_packet_to_key(packet, data_length, &key);
    if (!supported_protocol)
    {
        return;
    }

    net_callback_registry_handle_packet(&g_registry, key, packet);
}

bool ipv4_register_protocol(NetCallbackRegistryKey protocol_id_key, NetCallback callback, void *state)
{
    if (!g_is_registry_initalized)
    {
        initialize_registry();
    }

    return net_callback_registry_register(&g_registry, protocol_id_key, callback, state);
}

static bool g_have_ip = false;
static uint8_t g_my_ip[IPv4_IP_ADDR_SIZE];

bool ipv4_get_own_ip(uint8_t out_ip[static IPv4_IP_ADDR_SIZE])
{
    const uint64_t flags = get_cpu_flags();
    cli();

    if (!g_have_ip)
    {
        set_cpu_flags(flags);
        return false;
    }

    memmove(out_ip, g_my_ip, sizeof(g_my_ip));

    set_cpu_flags(flags);
    return true;
}

void ipv4_set_own_ip(const uint8_t ip[static IPv4_IP_ADDR_SIZE])
{
    const uint64_t flags = get_cpu_flags();
    cli();

    memmove(g_my_ip, ip, sizeof(g_my_ip));
    g_have_ip = true;

    set_cpu_flags(flags);
}
