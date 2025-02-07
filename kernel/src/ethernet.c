#include "ethernet.h"
#include "net.h"
#include "arp.h"
#include "ipv4.h"

void ethernet_handle_packet(EthernetPacket *packet, int data_length)
{
    switch (ntohs(packet->type))
    {
        case ethernet_TYPE_ARP:
            arp_handle(packet, data_length);
            break;
        case ethernet_TYPE_IPv4:
            ipv4_handle(packet, data_length);
            break;
        default:
            /* Drop */
            break;
    }
}
