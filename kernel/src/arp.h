#pragma once

#include <stdint.h>
#include "ethernet.h"
#include "ipv4.h"

// General ARP packet structure
// For IPv4/Ethernet see arp_IPv4EtherPacket
typedef struct {
    uint16_t hardware_type;
    uint16_t protocol_type;
    uint8_t hardware_length;
    uint8_t protocol_length;
    uint16_t operation;
    uint8_t content[];
} ArpPacket;

typedef struct {
    uint16_t hardware_type;  // = arp_HARDWARE_TYPE_ETHER
    uint16_t protocol_type;  // = arp_PROTOCOL_TYPE_IPv4
    uint8_t hardware_length; // = ETHER_MAC_SIZE
    uint8_t protocol_length; // = IPv4_IP_ADDR_SIZE
    uint16_t operation;
    uint8_t sender_mac[ETHER_MAC_SIZE];
    uint8_t sender_ip[IPv4_IP_ADDR_SIZE];
    uint8_t target_mac[ETHER_MAC_SIZE];
    uint8_t target_ip[IPv4_IP_ADDR_SIZE];
} arp_IPv4EtherPacket;

enum {
    arp_HARDWARE_TYPE_ETHER = 1,
    arp_PROTOCOL_TYPE_IPv4  = 0x0800,

    arp_OPERATION_REQUEST   = 1,
    arp_OPERATION_REPLY     = 2,
};

void arp_handle(EthernetPacket *packet, int data_length);
