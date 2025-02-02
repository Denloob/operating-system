#pragma once

#include <stdint.h>
#define IPv4_IP_ADDR_SIZE 4

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
