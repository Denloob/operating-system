#include "DHCP.h"
#include "assert.h"
#include "memory.h"
#include "string.h"
#include "io.h"


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
void dhcp_handle_offer(dhcp_packet_t *packet) 
{
    uint32_t offered_ip = packet->yiaddr;
    uint32_t server_ip = packet->siaddr;

    printf("DHCP Offer received!\n");
    printf("Offered IP: %d.%d.%d.%d\n", 
        (offered_ip >> 24) & 0xFF, 
        (offered_ip >> 16) & 0xFF, 
        (offered_ip >> 8) & 0xFF, 
        offered_ip & 0xFF);
    printf("DHCP Server IP: %d.%d.%d.%d\n", 
        (server_ip >> 24) & 0xFF, 
        (server_ip >> 16) & 0xFF, 
        (server_ip >> 8) & 0xFF, 
        server_ip & 0xFF);

    dhcp_send_request(offered_ip, server_ip, packet->xid);
}

void dhcp_send_request(uint32_t offered_ip, uint32_t server_ip, uint32_t xid)
{
    assert(false && "Not implemented");
}
