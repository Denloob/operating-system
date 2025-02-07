#include <stdint.h>

typedef struct 
{
    uint8_t op;         // Message type: 1 = request, 2 = reply
    uint8_t htype;      // Hardware type (Ethernet = 1)
    uint8_t hlen;       // Hardware address length (MAC = 6)
    uint8_t hops;       // Hops (always 0 for clients)
    uint32_t xid;       // Transaction ID
    uint16_t secs;      // Seconds elapsed
    uint16_t flags;     // Flags (0x8000 for broadcast)
    uint32_t ciaddr;    // Client IP (0.0.0.0 at this stage)
    uint32_t yiaddr;    // 'Your' IP (Offered IP)
    uint32_t siaddr;    // DHCP Server IP
    uint32_t giaddr;    // Relay agent IP
    uint8_t chaddr[16]; // Client MAC Address
    uint8_t sname[64];  // Server name
    uint8_t file[128];  // Boot file name
    uint8_t options[312]; // DHCP options (variable length)
} __attribute__((packed)) dhcp_packet_t;

void dhcp_handle_offer(dhcp_packet_t *packet);

void dhcp_send_request(uint32_t offered_ip, uint32_t server_ip, uint32_t xid);
