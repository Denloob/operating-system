#pragma once

#include "PCI.h"
#include "compiler_macros.h"
#include "ethernet.h"

#define rtl8139_VENDOR_ID 0x10ec
#define rtl8139_DEVICE_ID 0x8139

typedef struct {
    pci_DeviceAddress address;
    uint64_t io_base;
    uint8_t irq;
} rtl8139_Device;

#define res_rtl8139_DEVICE_NOT_FOUND "RTL8139 was not found"
#define res_rtl8139_UNEXPECTED_DEVICE_BEHAVIOR "RTL8139 behaved unexpectedly"
#define res_rtl8139_PACKETS_BUFFER_NO_MEMORY "Couldn't find enough memory to allocate the RTL8139 packets buffer"

// NOTE: after initializing call rtl8139_register_interrupt_handler
// @see rtl8139_register_interrupt_handler
res rtl8139_init() WUR;

/**
 * @brief - Register the RTL8139 interrupt handler.
 *
 * @param irq - The interrupt request number that the rtl8139 should use.
 */
void rtl8139_register_interrupt_handler(uint8_t irq);

/**
 * @brief - Try to transmit an Ethernet packet.
 *
 * @param packet - The Ethernet packet to transmit.
 * @param size   - The size of data in the packet to transmit.
 *                   NOTE: This means that we will read a total of `size + ETHER_PACKET_SIZE`
 *                      from `(char *)packet`!
 *
 * @return - true if the packet was transmitted. When false, this means that the
*              packet "queue" transmission is full, hence you should try again
*              some time later.
 */
bool rtl8139_try_transmit_packet(EthernetPacket *packet, int size);


void rtl8139_get_mac_address(uint8_t mac[6]);
