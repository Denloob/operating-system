#include "ipv4.h"
#include "net_callback_registry.h"
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
