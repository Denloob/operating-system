#pragma once

#include "compiler_macros.h"
#include "ethernet.h"
#include "ipv4.h"
#include <stdbool.h>
#include <stddef.h>

typedef void (*NetCallback)(void *state, void *packet);

typedef enum {
    net_callback_registry_PROTOCOL_ARP,
    net_callback_registry_PROTOCOL_UDP,
    net_callback_registry_PROTOCOL_TCP,
    net_callback_registry_PROTOCOL_ICMP,
    net_callback_registry_PROTOCOL_DHCP,
} NetCallbackRegistryKeyProtocolType;

typedef struct {
    uint16_t type; /* NetCallbackRegistryKeyProtocolType */

    union {
        struct {
            uint8_t ip[IPv4_IP_ADDR_SIZE];
            uint16_t port; // must be 0 if the protocol doesn't have a port.
        };

        uint8_t mac[ETHER_MAC_SIZE];
    };
} NetCallbackRegistryKey;

typedef struct {
    NetCallbackRegistryKey key;
    NetCallback callback; // if NULL, then the entry is empty
    void *state;
} NetCallbackRegistryEntry;

typedef struct {
    NetCallbackRegistryEntry *buf;
    size_t capacity;
} NetCallbackRegistry;

/**
 * @brief Initialize the given registry.
 *
 * @return true on success, false otherwise (false = out of memory)
 */
bool net_callback_registry_init(NetCallbackRegistry *registry) WUR;

/**
 * @brief - Register a packet handler, for the given key, with the given callback and state.
 *
 * @return - true on success, false otherwise (false = out of memory).
 */
bool net_callback_registry_register(NetCallbackRegistry *registry, NetCallbackRegistryKey key, NetCallback callback, void *state) WUR;

/**
 * @brief - Try to handle the given packet, with the given key, using one of the handlers in the registry.
 *              On success, calls and then removes the handler that it used. On
 *                  failure, does nothing.
 *
 * @return - true on success, false on failure.
 */
bool net_callback_registry_handle_packet(NetCallbackRegistry *registry, NetCallbackRegistryKey key, void *packet);
