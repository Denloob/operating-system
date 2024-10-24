#include "PCI.h"
#include "assert.h"
#include "io.h"

#define PORT_CONFIG_ADDRESS 0xCF8
#define PORT_CONFIG_DATA 0xCFC

static void pci_config_address(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) 
{
    assert((offset & 0b11) == 0 && "Offset must fe aligned to 4 bytes");
    assert((function & (~0x7)) == 0 && "Function has to be max 3 bits long");
    assert((device & (~0x1f)) == 0 && "");

    uint32_t address = (1 << 31)                 // Enable bit
                     | ((uint32_t)bus << 16)     // Bus number
                     | ((uint32_t)device << 11)  // Device number
                     | ((uint32_t)function << 8) // Function number
                     | (offset);                 // Offset (aligned to 4 bytes)
    out_dword(PORT_CONFIG_ADDRESS, address);
}

static uint32_t pci_config_data() 
{
    return in_dword(PORT_CONFIG_DATA);
}

uint32_t pci_config_read(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) 
{
    pci_config_address(bus, device, function, offset);
    return pci_config_data();
}

uint16_t pci_get_vendor_id(uint8_t bus, uint8_t device, uint8_t function) 
{
    return pci_config_read(bus, device, function, 0x00) & 0xFFFF;
}

uint16_t pci_get_device_id(uint8_t bus, uint8_t device, uint8_t function)
{
    return (pci_config_read(bus, device, function, 0x00) >> 16) & 0xFFFF;
}

uint8_t pci_get_class_code(uint8_t bus, uint8_t device, uint8_t function) 
{
    return (pci_config_read(bus, device, function, 0x08) >> 24) & 0xFF;
}

uint8_t pci_get_subclass_code(uint8_t bus, uint8_t device, uint8_t function) 
{
    return (pci_config_read(bus, device, function, 0x08) >> 16) & 0xFF;
}

uint32_t pci_get_bar(uint8_t bus, uint8_t device, uint8_t function, uint8_t bar_index) 
{
    return pci_config_read(bus, device, function, 0x10 + (bar_index * 4));
}

void pci_scan_for_ide()
{
    for (uint16_t bus = 0; bus < 256; bus++) 
    {
        for (uint8_t device = 0; device < 32; device++) 
        {
            uint16_t vendor_id = pci_get_vendor_id(bus, device, 0);
            if (vendor_id == 0xFFFF) continue; // Device doesn't exist

            uint8_t class_code = pci_get_class_code(bus, device, 0);
            uint8_t subclass_code = pci_get_subclass_code(bus, device, 0);

            if (class_code == 0x01 && subclass_code == 0x01) //check for IDE controller
            { 
                printf("IDE controller found at bus %u, device %u\n", bus, device);
                // Print BARs
                for (uint8_t bar = 0; bar < 6; bar++) {
                    uint32_t bar_value = pci_get_bar(bus, device, 0, bar);
                    if (bar_value) {
                        printf("BAR%u: 0x%x\n", bar, bar_value);
                    }
                }
            }
        }
    }
}

