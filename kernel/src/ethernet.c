#include "ethernet.h"

void ethernet_handle_packet(EthernetPacket *packet, int data_length)
{
    switch (packet->type)
    {
        case ethernet_TYPE_ARP:
            // TODO:
            break;
        case ethernet_TYPE_IPv4:
            // TODO:
            break;
        default:
            /* Drop */
            break;
    }
}
