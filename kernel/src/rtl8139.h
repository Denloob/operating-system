#pragma once

#include "PCI.h"
#include "compiler_macros.h"

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
