#include <stdint.h>
#include <limits.h>
#include "rtl8139.h"
#include "PCI.h"
#include "assert.h"
#include "io.h"
#include "kmalloc.h"
#include "mmap.h"
#include "mmu.h"

#define rtl8139_IO_BASE_BAR_INDEX 0

static rtl8139_Device g_rtl;

#define rtl_read(size, offset)                                                 \
    in_ ## size (g_rtl.io_base + offset)

#define rtl_write(size, offset, value)                                         \
    out_ ## size (g_rtl.io_base + offset, value)

enum {
    // I/O Registers
    rtl8139_REG_MAC0_5   = 0x0, /* MAC0-5; 6 bytes */
    rtl8139_REG_MAR0_7   = 0x8,         /* 8 bytes */
    rtl8139_REG_RBSTART  = 0x30, /* Receive buffer beginning;
                                           4 bytes */
    rtl8139_REG_CMD      = 0x37,        /* 1 bytes */

    rtl8139_REG_IMR      = 0x3c, /* Interrupt mask register;    2 bytes */
    rtl8139_REG_ISR      = 0x3e, /* Interrupt service register; 2 bytes */

    rtl8139_REG_RCR      = 0x44,        /* 4 bytes */

    rtl8139_REG_CONFIG_1 = 0x52,        /* 1 byte  */

    // Receive Configuration Register bits
    rtl8139_RCR_ACCEPT_ALL              = (1 << 0),
    rtl8139_RCR_ACCEPT_PHYSICAL_MATCH   = (1 << 1),
    rtl8139_RCR_ACCEPT_MULTICAST        = (1 << 2),
    rtl8139_RCR_ACCEPT_BROADCAST        = (1 << 3),
    rtl8139_RCR_WRAP                    = (1 << 7), // Will "overflow" the Rx buffer by 1500 bytes, if needed. We will reserve 1500 bytes in the buffer.

    // Receive Status Register bits
    rtl8139_RSR_RECEIVE_OK       = (1 << 0),
    rtl8139_RSR_BROADCAST        = (1 << 13),
    rtl8139_RSR_PHYSICAL_ADDR    = (1 << 14),
    rtl8139_RSR_MULTICAST        = (1 << 15),

    // Commands
    rtl8139_CMD_RESET               = (1 << 4),
    rtl8139_CMD_RECEIVER_ENABLE     = (1 << 3),
    rtl8139_CMD_TRANSMITTER_ENABLE  = (1 << 2),

    // Mask bits
    rtl8139_IMR_TRANSMIT_ERR     = (1 << 3),
    rtl8139_IMR_TRANSMIT_OK      = (1 << 2),
    rtl8139_IMR_RECEIVE_ERR      = (1 << 1),
    rtl8139_IMR_RECEIVE_OK       = (1 << 0),
};

static void rtl8139_enable_bus_master()
{
    pci_set_bus_master(g_rtl.address);
}

static res rtl8139_find_rtl(rtl8139_Device *out)
{
    res rs = pci_find_device(rtl8139_VENDOR_ID, rtl8139_DEVICE_ID, &out->address);
    if (!IS_OK(rs))
    {
        return res_rtl8139_DEVICE_NOT_FOUND;
    }

    out->io_base = pci_get_bar(out->address, rtl8139_IO_BASE_BAR_INDEX) & ~1; // Unset the LSB marking the BAR as I/O space address.
    if (out->io_base == 0)
    {
        return res_rtl8139_UNEXPECTED_DEVICE_BEHAVIOR;
    }

    return res_OK;
}

static void rtl8139_power_on()
{
    rtl_write(byte, rtl8139_REG_CONFIG_1, 0x0);
}

static void rtl8139_software_reset()
{
    rtl_write(byte, rtl8139_REG_CMD, rtl8139_CMD_RESET);

    int rst_bit;
    do
    {
        rst_bit = rtl_read(byte, rtl8139_REG_CMD) & rtl8139_CMD_RESET;
    } while (rst_bit != 0);
}

// TODO: randomize these at runtime
#define RECV_BUFFER_CANARY1 0xbd2410f129cb24a1
#define RECV_BUFFER_CANARY2 0x068b9a4fda74e441

#define RECV_BUFFER_SIZE (8192 + 16 + 1500 + 16) // Smallest buffer size supported by rtl8139 + 1500 "overflow" + 16 magic bytes to detect actual overflows
static char *g_recv_buffer;

static void write_recv_buffer_canary()
{
    *(uint64_t *)(&g_recv_buffer[RECV_BUFFER_SIZE - 16]) = RECV_BUFFER_CANARY1;
    *(uint64_t *)(&g_recv_buffer[RECV_BUFFER_SIZE - 8]) = RECV_BUFFER_CANARY2;
}

__attribute__((unused)) // TODO: use when reading the buffer
static void validate_recv_buffer_canary()
{
    assert(*(uint64_t *)(&g_recv_buffer[RECV_BUFFER_SIZE - 16]) == RECV_BUFFER_CANARY1);
    assert(*(uint64_t *)(&g_recv_buffer[RECV_BUFFER_SIZE - 8]) == RECV_BUFFER_CANARY2);
}

static void rtl8139_init_recv_buffer()
{
    g_recv_buffer = kmalloc(RECV_BUFFER_SIZE);
    write_recv_buffer_canary();

    mmu_PageTableEntry *page = mmu_page_existing(g_recv_buffer);
    uint64_t physical_buffer_addr = mmu_page_table_entry_address_get(page);
    assert(physical_buffer_addr < (UINT32_MAX - RECV_BUFFER_SIZE) && "Physical memory of the buffer must be inside the 32 bit range");

    rtl_write(dword, rtl8139_REG_RBSTART, physical_buffer_addr);
}

static void rtl8139_set_interrupt_mask(uint16_t value)
{
    rtl_write(word, rtl8139_REG_IMR, value);
}

static void rtl8139_set_receive_config(uint32_t value)
{
    rtl_write(dword, rtl8139_REG_RCR, value);
}

static void rtl8139_enable_transmit_and_receive()
{
    rtl_write(byte, rtl8139_REG_CMD, rtl8139_CMD_RECEIVER_ENABLE | rtl8139_CMD_TRANSMITTER_ENABLE);
}

res rtl8139_init()
{
    res rs = rtl8139_find_rtl(&g_rtl);
    if (!IS_OK(rs))
    {
        return rs;
    }

    rtl8139_enable_bus_master();
    rtl8139_power_on();
    rtl8139_software_reset();

    rtl8139_init_recv_buffer();

    rtl8139_set_interrupt_mask(rtl8139_IMR_RECEIVE_OK | rtl8139_IMR_RECEIVE_ERR |
                               rtl8139_IMR_TRANSMIT_OK | rtl8139_IMR_TRANSMIT_ERR);

    rtl8139_set_receive_config(rtl8139_RCR_ACCEPT_PHYSICAL_MATCH |
                               rtl8139_RCR_ACCEPT_BROADCAST | rtl8139_RCR_WRAP);

    rtl8139_enable_transmit_and_receive();

    return res_OK;
}
