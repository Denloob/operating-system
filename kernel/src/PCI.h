#pragma once

#include "res.h"
#include <stdint.h>
#include <stdbool.h>


/* 
 * What is PCI?
 *
 * PCI is a standard that defines how devices communicate with the CPU.
 * In general it allows devices such as the IDE controller, graphics cards, and network cards to connet to the mother board.
 *
 * Each PCI device has a configuration space, which includes information like vendor ID,
 * device ID, class codes, and base address registers (BARs)
 *
 * vendor id - A 16-bit identifier that indicates which company created the PCI device 
 *
 * Device ID - A 16-bit identifier for the spesifc device model 
 *
 * for example a vector id of 0x8086 and device id of 0x100E will indicate a intel etthernet controller device
 *
 * command register - A register that allows the system to control the device operation , for exmaple bit 0 is for I/O space enable , bit 1 is for memory space ...
 *
 * ...
 *
 * Class code - indicates the general type of the device , 0x01 - mass storage controller , 0x02 - network controller , 0x03 - display controller ...
 *
 * subclass code - indicates the spesific type of the device , 0x01 - IDE controller , 0x00 VGA-compatible controller
 *
 * Base address registers - Six registers used to map the device's memory or I/O into the system's space address , each bar is 32bits and each device will have 6 of them
 *
 * What is IDE?
 *
 * IDE is a PCI device that connects storage devices like hard drives to the mother board , the IDE controllers manage data transfer between the storage devices and the system.
 *
 * IDE has 2 main modes PIO , DMA
 *
 * PIO - The CPU is directly involved in the transfer of data between the storage device and memory.
 * DMA - The CPU is less involved, and the controller can transfer data directly to memory, freeing up CPU time. .
 */

typedef struct {
    uint8_t bus;
    uint8_t device;
    uint8_t function;
} pci_DeviceAddress;

/*
 * @brief - Reads a 32-bit value from the PCI configuration space at the given bus, device, function, and offset.
 * 
 * @bus - The PCI bus number (0-255).
 * @device - The PCI device number on the bus (0-31).
 * @function - The PCI function number (0-7).
 * @offset - The offset into the PCI configuration space (must be aligned to 4 bytes).
 * 
 * @return - The 32-bit value from the configuration space.
 */
uint32_t pci_config_read(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);

/*
 * @brief - Retrieves the Vendor ID from the PCI configuration space for the given device.
 * 
 * @Vendor ID -  A 16-bit data (explained at top).
 * 
 * @return -The 16-bit vendor ID.
 */
uint16_t pci_get_vendor_id(uint8_t bus, uint8_t device, uint8_t function);

/*
 * @brief - Retrieves the Device ID from the PCI configuration space for the given device.
 * 
 * @Device ID - A 16-bit data
 *
 * @return - The 16-bit device ID.
 */
uint16_t pci_get_device_id(uint8_t bus, uint8_t device, uint8_t function);

/**
 * @brief Like pci_get_device_id and pci_get_vendor_id but retrieves both in one
 *          PCI operation.
 * 
 * @return `(device_id << 16) | (vendor_id)`. Both ids are 16 bits.
 */
uint32_t pci_get_device_and_vendor_id(uint8_t bus, uint8_t device, uint8_t function);

/*
 * @brief - Retrieves the Class Code from the PCI configuration space for the given device.
 * 
 * @Class Code -  A 8-bit data
 * 
 * @returns - The 8-bit class code.
 */
uint8_t pci_get_class_code(uint8_t bus, uint8_t device, uint8_t function);

/*
 * @brief - Retrieves the Subclass Code from the PCI configuration space for the given device.
 * 
 * @Subclass Code - A 8-bit .
 * 
 * @return - The 8-bit subclass code.
 */
uint8_t pci_get_subclass_code(uint8_t bus, uint8_t device, uint8_t function);

/*
 * @brief - Reads a Base Address Register (BAR) from the PCI configuration space.
 * 
 * Bars - These registers define the I/O or memory addresses used to interact with the device.
 * 
 * @bus - The PCI bus number.
 * @device - The PCI device number.
 * @function - The PCI function number.
 * @bar_index -The index of the BAR to read (0-5).
 * 
 * @return - The 32-bit BAR value, which can be either a memory-mapped or I/O port address.
 */
uint32_t pci_get_bar(uint8_t bus, uint8_t device, uint8_t function, uint8_t bar_index);

/*
 * @brief: 
 * 
 *  Scans the PCI bus for IDE controllers. 
 * 
 * IDE (Integrated Drive Electronics) is a standard for interfacing storage devices like hard drives.
 * An IDE controller connects to the system through the PCI bus and controls data transfers between
 * the CPU and the storage device.
 *
 * This function identifies PCI devices with a Class Code of 0x01 and a Subclass Code of 0x01 (IDE).
 * 
 * It prints the bus, device, and function number for each IDE controller found, as well as its BARs.
 */
void pci_scan_for_ide();

#define res_pci_DEVICE_NOT_FOUND "No PCI device was found for the given query"

/**
 * @brief Find a PCI device with the given vendor ID and device ID.
 *
 * @return res_OK or one of the errors above.
 */
res pci_find_device(uint16_t vendor_id, uint16_t device_id, pci_DeviceAddress *out);
