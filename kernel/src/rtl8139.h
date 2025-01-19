#pragma once

#include "PCI.h"
#include "compiler_macros.h"

#define rtl8139_VENDOR_ID 0x10ec
#define rtl8139_DEVICE_ID 0x8139

typedef struct {
    pci_DeviceAddress address;
    uint64_t io_base;
} rtl8139_Device;

#define res_rtl8139_DEVICE_NOT_FOUND "RTL8139 was not found"
#define res_rtl8139_UNEXPECTED_DEVICE_BEHAVIOR "RTL8139 behaved unexpectedly"

res rtl8139_init() WUR;
