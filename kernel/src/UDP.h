#pragma once

#include <stdint.h>

typedef struct __attribute__((packed))
{
    uint16_t src_port;
    uint16_t dest_port;
    uint16_t length;
    uint16_t checksum;
    uint8_t  data[];
} UDPHeader;


void udp_send_packet(uint8_t *dest_ip, uint16_t src_port, uint16_t dest_port, uint8_t *data, uint16_t data_length);
