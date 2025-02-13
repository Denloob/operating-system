#include "ethernet.h"
#include "net.h"
#include "arp.h"
#include "ipv4.h"
#include "UDP.h"

void ethernet_handle_packet(EthernetPacket *packet, int data_length)
{
    uint16_t ethertype = ntohs(packet->ethertype);

    switch (ethertype)
    {
        case ethernet_TYPE_ARP:
            arp_handle(packet, data_length);
            break;

        case ethernet_TYPE_IPv4:
        {
            IPv4Packet *ip_packet = (IPv4Packet *)(packet->data_and_fcs);
            uint8_t protocol = ip_packet->protocol;

            switch (protocol)
            {
                case 1://ICMP
                    //icmp_handle();
                    break;

                case 17: //UDP
                {
                    UDPHeader *udp_header = (UDPHeader *)(ip_packet->option_and_data);
                    uint16_t src_port = ntohs(udp_header->src_port);
                    uint16_t dest_port = ntohs(udp_header->dest_port);
#define DHCP_CLIENT_PORT 68
#define DHCP_SERVER_PORT 67
                    if (dest_port == DHCP_CLIENT_PORT || dest_port == DHCP_SERVER_PORT)
                    {
                        //dhcp_handle();
                    }
                    else
                    {
                        //udp_handle();
                    }
                    break;
                }
                default:
                    /*protocol is not supported or something*/
                    break;
            }
            break;
        }

        default:
            /* Drop*/
            break;
    }
}
