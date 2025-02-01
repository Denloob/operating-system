#include "PCI.h"
#include "assert.h"
#include "io.h"
#include "pic.h"
#include "res.h"

#define PORT_CONFIG_ADDRESS 0xCF8
#define PORT_CONFIG_DATA 0xCFC

static void pci_config_address(pci_DeviceAddress address, uint8_t offset) 
{
    assert((offset & 0b11) == 0 && "Offset must fe aligned to 4 bytes");
    assert((address.function & (~0x7)) == 0 && "Function has to be max 3 bits long");
    assert((address.device & (~0x1f)) == 0 && "");

    uint32_t numerical_address = (1 << 31)     // Enable bit
                     | (address.bus << 16)     // Bus number
                     | (address.device << 11)  // Device number
                     | (address.function << 8) // Function number
                     | (offset);               // Offset (aligned to 4 bytes)
    out_dword(PORT_CONFIG_ADDRESS, numerical_address);
}

static uint32_t pci_config_data() 
{
    return in_dword(PORT_CONFIG_DATA);
}

static void pci_config_data_write(uint32_t value) 
{
    out_dword(PORT_CONFIG_DATA, value);
}

// NOTE: We are writing **32 bit** values! For example, writing to offset 0x4
//          will overwrite both status and command!
void pci_config_write(pci_DeviceAddress address, uint8_t offset, uint32_t value)
{
    pci_config_address(address, offset);
    pci_config_data_write(value);
}

uint32_t pci_config_read(pci_DeviceAddress address, uint8_t offset) 
{
    pci_config_address(address, offset);
    return pci_config_data();
}

uint8_t pci_get_header_type(pci_DeviceAddress address)
{
    return (pci_config_read(address, 0xc) >> 16) & 0xff;
}

#define INTERRUPT_LINE_OFFSET 0x3c

uint8_t pci_get_irq_number(pci_DeviceAddress address)
{
    // NOTE: applicable only to type 0 header type.
    // @see pci_get_header_type
    return pci_config_read(address, INTERRUPT_LINE_OFFSET) & 0xff;
}

void pci_set_irq_number(pci_DeviceAddress address, uint8_t irq)
{
    assert(irq <= pic_IRQ_SECONDARY_ATA_OR_SPURIOUS && "IRQ number is out of range");
    // NOTE: applicable only to type 0 header type.
    // @see pci_get_header_type
    uint32_t old_value = pci_config_read(address, INTERRUPT_LINE_OFFSET);
    uint32_t new_value = (old_value & ~0xff) | irq;
    pci_config_write(address, INTERRUPT_LINE_OFFSET, new_value);
}

uint16_t pci_get_vendor_id(pci_DeviceAddress address) 
{
    return pci_config_read(address, 0x00) & 0xFFFF;
}

uint16_t pci_get_device_id(pci_DeviceAddress address)
{
    return (pci_config_read(address, 0x00) >> 16) & 0xFFFF;
}

uint32_t pci_get_device_and_vendor_id(pci_DeviceAddress address)
{
    return pci_config_read(address, 0x00);
}

#define CMD_STATUS_OFFSET 0x4

void pci_set_bus_master(pci_DeviceAddress address)
{
#define BUS_MASTER_BIT 2
    uint32_t old_value = pci_config_read(address, CMD_STATUS_OFFSET);
    uint32_t new_value = old_value | (1 << BUS_MASTER_BIT);
    pci_config_write(address, CMD_STATUS_OFFSET, new_value);
}

uint8_t pci_get_class_code(pci_DeviceAddress address) 
{
    return (pci_config_read(address, 0x08) >> 24) & 0xFF;
}

uint8_t pci_get_subclass_code(pci_DeviceAddress address) 
{
    return (pci_config_read(address, 0x08) >> 16) & 0xFF;
}

uint32_t pci_get_bar(pci_DeviceAddress address, uint8_t bar_index) 
{
    return pci_config_read(address, 0x10 + (bar_index * 4));
}

res pci_find_device(uint16_t vendor_id, uint16_t device_id, pci_DeviceAddress *out)
{
    assert(out != NULL);

    uint32_t target_vendor_and_device = (device_id << 16) | vendor_id;
    for (int bus = 0; bus < 256; bus++)
    {
        for (int device = 0; device < 32; device++)
        {
            for (int function = 0; function < 8; function++)
            {
                uint32_t vendor_and_device = pci_get_device_and_vendor_id((pci_DeviceAddress){bus, device, function});
                if (target_vendor_and_device == vendor_and_device)
                {
                    out->bus = bus;
                    out->device = device;
                    out->function = function;

                    return res_OK;
                }
            }
        }
    }

    return res_pci_DEVICE_NOT_FOUND;
}

void pci_scan_for_ide()
{
    for (uint16_t bus = 0; bus < 256; bus++) 
    {
        for (uint8_t device = 0; device < 32; device++) 
        {
            for(uint8_t function=0; function<8;function++)
            {
                pci_DeviceAddress address = {bus, device, function};
                uint16_t vendor_id = pci_get_vendor_id(address);
                if (vendor_id == 0xFFFF) continue; // Device doesn't exist

                uint8_t class_code = pci_get_class_code(address);
                uint8_t subclass_code = pci_get_subclass_code(address);

                if (class_code == 0x01 && subclass_code == 0x01) //check for IDE controller
                { 
                    printf("IDE controller found at bus 0x%x, device 0x%x , function 0x%x\n", bus, device , function);
                    // Print BARs
                    for (uint8_t bar = 0; bar < 6; bar++)
                    {
                        uint32_t bar_value = pci_get_bar(address, bar);
                        if (bar_value)
                        {
                            printf("BAR 0x%x: 0x%x\n", bar, bar_value);
                        }
                    }
                }
            }
        }
    }
}
