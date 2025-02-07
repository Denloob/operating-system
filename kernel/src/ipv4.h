#pragma once

#include "ethernet.h"
#include "net_callback_registry.h"
#include "ipv4_constants.h"
#include <stdint.h>

typedef struct {
    uint8_t     ihl : 4; // The order is `version, ihl` in big endian. We are in little endian, so it's reversed
    uint8_t     version : 4;

    uint8_t     type_of_service;
    uint16_t    total_length;
    uint16_t    id;
    uint16_t    fragment_offset;
    uint8_t     ttl;
    uint8_t     protocol;
    uint16_t    checksum;
    
    uint8_t     source_ip[IPv4_IP_ADDR_SIZE];
    uint8_t     dest_ip[IPv4_IP_ADDR_SIZE];
    uint8_t     option_and_data[];
} IPv4Packet;

void ipv4_handle(EthernetPacket *packet, int data_length);

// @return - true on success, false otherwise (false = out of memory)
bool ipv4_register_protocol(NetCallbackRegistryKey protocol_id_key, NetCallback callback, void *state);

// @return - true if we have an IP and it was written, false otherwise.
bool ipv4_get_own_ip(uint8_t out_ip[static IPv4_IP_ADDR_SIZE]);

void ipv4_set_own_ip(const uint8_t ip[static IPv4_IP_ADDR_SIZE]);

__attribute__((pure))
static inline NetCallbackRegistryKey ipv4_create_protocol_key_udp(uint8_t ip[IPv4_IP_ADDR_SIZE], uint16_t port)
{
    return (NetCallbackRegistryKey){
        .type = net_callback_registry_PROTOCOL_UDP,
        .ip = {ip[0], ip[1], ip[2], ip[3]},
        .port = port,
    };
}

__attribute__((pure))
static inline NetCallbackRegistryKey ipv4_create_protocol_key_tcp(uint8_t ip[IPv4_IP_ADDR_SIZE], uint16_t port)
{
    return (NetCallbackRegistryKey){
        .type = net_callback_registry_PROTOCOL_TCP,
        .ip = {ip[0], ip[1], ip[2], ip[3]},
        .port = port,
    };
}

__attribute__((pure))
static inline NetCallbackRegistryKey ipv4_create_protocol_key_icmp(uint8_t ip[IPv4_IP_ADDR_SIZE])
{
    return (NetCallbackRegistryKey){
        .type = net_callback_registry_PROTOCOL_ICMP,
        .ip = {ip[0], ip[1], ip[2], ip[3]},
    };
}

__attribute__((pure))
static inline NetCallbackRegistryKey ipv4_create_protocol_key_dhcp(uint8_t mac[ETHER_MAC_SIZE])
{
    return (NetCallbackRegistryKey){
        .type = net_callback_registry_PROTOCOL_ICMP,
        .mac = {
            mac[0],
            mac[1],
            mac[2],
            mac[3],
            mac[4],
            mac[5],
        },
    };
}
