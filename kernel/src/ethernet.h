#pragma once

#include <stdint.h>

#define ETHER_MAC_SIZE 6

typedef struct __attribute__((packed)) {
    uint8_t dest[ETHER_MAC_SIZE];
    uint8_t source[ETHER_MAC_SIZE];
    uint16_t type;
    uint8_t data_and_fcs[]; // Data (46-1500 bytes) and the 4 byte fcs.
} EthernetPacket;

#define ETHER_PACKET_SIZE (sizeof(EthernetPacket) + 4) // +4 for fcs

enum {
    ethernet_TYPE_IPv4 = 0x0800,
    ethernet_TYPE_ARP  = 0x0806,
};

void ethernet_handle_packet(EthernetPacket *packet, int data_length);
