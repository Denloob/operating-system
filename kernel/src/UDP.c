#include "UDP.h"
#include "ethernet.h"
#include "ipv4.h"
#include "memory.h"
#include "rtl8139.h"
#include "net.h"

void udp_send_packet(uint8_t *dest_ip, uint16_t src_port, uint16_t dest_port, uint8_t *data, uint16_t data_length)
{
    uint16_t total_udp_length = sizeof(UDPHeader) + data_length;
    uint16_t total_ip_length = sizeof(IPv4Packet) + total_udp_length;
    uint16_t total_ethernet_length = sizeof(EthernetPacket) + total_ip_length;

    uint8_t packet[total_ethernet_length];
    memset(packet, 0, sizeof(packet));

    // Ethernet Header
    EthernetPacket *eth = (EthernetPacket *)packet;
    memset(eth->dest_mac, 0xFF, 6);
    
    uint8_t mac[6];
    rtl8139_get_mac_address(mac);
    
    memmove(eth->source_mac, mac, 6);
    eth->ethertype = htons(ethernet_TYPE_IPv4);

    // IP Header
    IPv4Packet *ip = (IPv4Packet *)eth->data_and_fcs;
    ip->version = 4;
    ip->ihl = 5;
    ip->ttl = 64;
    ip->protocol = IPv4_PROTOCOL_UDP;
    ip->total_length = htons(total_ip_length);

    uint8_t my_ip[4] = {10, 0, 2, 15};
    memmove(ip->source_ip, my_ip, IPv4_IP_ADDR_SIZE);
    memmove(ip->dest_ip, dest_ip, IPv4_IP_ADDR_SIZE);

    // UDP Header
    UDPHeader *udp = (UDPHeader *)ip->option_and_data;
    udp->src_port = htons(src_port);
    udp->dest_port = htons(dest_port);
    udp->length = htons(total_udp_length);
    memmove(udp->data, data, data_length);

    rtl8139_try_transmit_packet((EthernetPacket *)packet, total_ethernet_length);
}
