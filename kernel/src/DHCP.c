#include "DHCP.h"
#include "assert.h"
#include "memory.h"
#include "string.h"
#include "io.h"
#include "net.h"
#include "rtl8139.h"
#include "UDP.h"
#include "ipv4.h"

#define DHCP_CLIENT_PORT 68
#define DHCP_SERVER_PORT 67

/*  DHCP stages:
 * server discovery - the client sends a broadcast and expects a return from a dhcp server to get an ip
 *
 * ip offering - the server responds with an ip it offers to the client
 *
 * ip request - the client tells the DHCP server it wants the ip it was Offered
 *
 * acknowledgement - the server tells the client it accepts the "transaction"
 *
 * */

void dhcp_server_discovery() 
{
    dhcp_packet_t discover_packet;
    memset(&discover_packet, 0, sizeof(dhcp_packet_t));
    
    discover_packet.op = 1; // Boot Request
    discover_packet.htype = 1; // Ethernet
    discover_packet.hlen = 6;
    discover_packet.hops = 0;
    discover_packet.xid = 0x12345678; // Example Transaction ID
    discover_packet.secs = 0;
    discover_packet.flags = htons(0x8000); // Broadcast
    
    uint8_t mac[6];
    rtl8139_get_mac_address(mac);
    memmove(discover_packet.chaddr, mac, 6);
    
    uint8_t *options = discover_packet.options;
    options[0] = 0x63; options[1] = 0x82;
    options[2] = 0x53; options[3] = 0x63;
    
    options[4] = 53;  // DHCP Message Type
    options[5] = 1;
    options[6] = 1;  // DHCPDISCOVER
    
    options[7] = 255; // End
    
    uint8_t dest_ip[4] = {255, 255, 255, 255};
    udp_send_packet(dest_ip, DHCP_CLIENT_PORT, DHCP_SERVER_PORT, (uint8_t *)&discover_packet, sizeof(dhcp_packet_t));
    
    printf("DHCP Discover broadcast sent.\n");
}

void dhcp_recv_offer(DHCPOffer *offer , dhcp_packet_t *packet)
{
    offer->ip = packet->yiaddr;
    offer->server_ip = packet->siaddr;
    offer->xid = packet->xid;
    
    printf("DHCP Offer received! Offered IP: %d.%d.%d.%d\n",
        (offer->ip >> 24) & 0xFF, (offer->ip >> 16) & 0xFF,
        (offer->ip >> 8) & 0xFF, offer->ip & 0xFF);
}

void dhcp_send_request(uint32_t offered_ip, uint32_t server_ip, uint32_t xid)
{
    dhcp_packet_t request_packet;
    memset(&request_packet, 0, sizeof(dhcp_packet_t));

    // DHCP Header
    request_packet.op = 1;  // Boot Request
    request_packet.htype = 1;  //Ethernet
    request_packet.hlen = 6;
    request_packet.hops = 0;
    request_packet.xid = xid;
    request_packet.secs = 0;
    request_packet.flags = htons(0x8000);  //Broadcast
    request_packet.ciaddr = 0;
    request_packet.yiaddr = 0;
    request_packet.siaddr = 0;
    request_packet.giaddr = 0;

    // Set MAC Address
    uint8_t mac[6];
    rtl8139_get_mac_address(mac);
    memmove(request_packet.chaddr, mac, 6);

    uint8_t *options = request_packet.options;
    options[0] = 0x63; options[1] = 0x82;
    options[2] = 0x53; options[3] = 0x63;

    // DHCP Message Type: Request
    options[4] = 53;  // Option: DHCP Message Type
    options[5] = 1;   // Length: 1
    options[6] = 3;   // DHCPREQUEST

    // Requested IP Address
    options[7] = 50;  // Option: Requested IP Address
    options[8] = 4;   // Length: 4
    memmove(&options[9], &offered_ip, 4);

    // DHCP Server Identifier
    options[13] = 54;  // Option: DHCP Server Identifier
    options[14] = 4;   // Length: 4
    memmove(&options[15], &server_ip, 4);

    options[19] = 255;

    uint8_t dest_ip[4] = {255, 255, 255, 255};  // Broadcast
    udp_send_packet(dest_ip, DHCP_CLIENT_PORT, DHCP_SERVER_PORT, (uint8_t *)&request_packet, sizeof(dhcp_packet_t));

    printf("DHCP Request sent for IP: %d.%d.%d.%d\n",
        (offered_ip >> 24) & 0xFF,
        (offered_ip >> 16) & 0xFF,
        (offered_ip >> 8) & 0xFF,
        offered_ip & 0xFF);
}

void dhcp_accept_offer(DHCPOffer *offer)
{
    dhcp_send_request(offer->ip, offer->server_ip, offer->xid);
}

void dhcp_recv_ack(DHCPAck *ack , dhcp_packet_t *packet) 
{
    for (int i = 0; i < 312; i++) 
    {
        if (packet->options[i] == 53 && packet->options[i + 2] == 5)  // DHCPACK
        {
            ack->ip[0] = (packet->yiaddr >> 24) & 0xFF;
            ack->ip[1] = (packet->yiaddr >> 16) & 0xFF;
            ack->ip[2] = (packet->yiaddr >> 8) & 0xFF;
            ack->ip[3] = packet->yiaddr & 0xFF;
            return;
        }
    }
}

