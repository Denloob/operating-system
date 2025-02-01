#include <stdint.h>
#include <limits.h>
#include "rtl8139.h"
#include "IDT.h"
#include "PCI.h"
#include "assert.h"
#include "io.h"
#include "isr.h"
#include "kmalloc.h"
#include "math.h"
#include "memory.h"
#include "mmap.h"
#include "mmu.h"
#include "pic.h"
#include "smartptr.h"

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

    rtl8139_REG_CAPR     = 0x38, /* Current Address of Packet Read (end of last packet read by us);             2 bytes */
    rtl8139_REG_CBA      = 0x3a, /* aka CBR, Current Buffer Address (end of last received packet by rtl8139);   2 bytes */

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

    out->irq = pci_get_irq_number(out->address);

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

#define RECV_BUFFER_SIZE_RAW (8192) // The raw size of the rx buffer, excluding all the additional stuff like in RECV_BUFFER_SIZE. Should be used only for the ring buffer calculations.
#define RECV_BUFFER_SIZE (8192 + 16 + 1500) // Smallest buffer size supported by rtl8139 + 1500 "overflow"

// Structure describing all the buffers for the transmit/receive packets
//  needed by the RTL.
// We put them all in one struct so we can allocate their physical memory
//  contiguously easily without losing data on alignment.
typedef struct {
    struct {
        char buffer[RECV_BUFFER_SIZE];

        // The last location from which we read from the recv buffer. Should be CAPR+0x10
        uint16_t ptr;
    } rx;

    uint64_t canary1;
    uint64_t canary2;
} rtl_PacketBuffers;

#define PACKET_BUFFER_VIRTUAL_ADDR 0xffff813900000000
static rtl_PacketBuffers *g_packet_buffers;

static void write_recv_buffer_canary()
{
    g_packet_buffers->canary1 = RECV_BUFFER_CANARY1;
    g_packet_buffers->canary2 = RECV_BUFFER_CANARY2;
}

__attribute__((unused))
static void validate_recv_buffer_canary()
{
    assert(g_packet_buffers->canary1 == RECV_BUFFER_CANARY1);
    assert(g_packet_buffers->canary2 == RECV_BUFFER_CANARY2);
}

static bool rtl8139_allocate_packet_buffers_phys(uint64_t *out_phys_address, uint64_t size)
{
    assert(out_phys_address);
    bool success = mmap_allocate_contiguous(size, out_phys_address);
    return success;
}

WUR
static res rtl8139_init_recv_buffer()
{
    uint64_t physical_buffer_addresses_addr;
    const uint64_t buffer_addresses_size = PAGE_ALIGN_UP(sizeof(*g_packet_buffers));
    bool success = rtl8139_allocate_packet_buffers_phys(&physical_buffer_addresses_addr,
                                                        buffer_addresses_size);
    if (!success)
    {
        return res_rtl8139_PACKETS_BUFFER_NO_MEMORY;
    }

    mmu_map_range(physical_buffer_addresses_addr,
                  physical_buffer_addresses_addr + buffer_addresses_size,
                  PACKET_BUFFER_VIRTUAL_ADDR, MMU_EXECUTE_DISABLE | MMU_READ_WRITE);

    g_packet_buffers = (void *)PACKET_BUFFER_VIRTUAL_ADDR;

    write_recv_buffer_canary();

    rtl_write(dword, rtl8139_REG_RBSTART, physical_buffer_addresses_addr);

    return res_OK;
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

#define INTERRUPT_MASK rtl8139_IMR_RECEIVE_OK | rtl8139_IMR_RECEIVE_ERR | rtl8139_IMR_TRANSMIT_OK | rtl8139_IMR_TRANSMIT_ERR

typedef struct {
    uint32_t receive_ok : 1;
    uint32_t err_frame_alignment : 1;
    uint32_t err_crc : 1;
    uint32_t long_packet : 1;
    uint32_t runt_packet : 1;
    uint32_t err_invalid_symbol : 1;
    uint32_t reserved : 7;
    uint32_t is_broadcast : 1;
    uint32_t is_physical_mac_match : 1;
    uint32_t is_multicast : 1;
    uint32_t packet_size : 16;
} ReceiveHeader;

void rtl8139_recv_packer(uint16_t status)
{
    assert(status & rtl8139_IMR_RECEIVE_OK);
    validate_recv_buffer_canary();

    const char *rx_buf = g_packet_buffers->rx.buffer;
    const ReceiveHeader *header = (ReceiveHeader *)&rx_buf[  g_packet_buffers->rx.ptr                    ];
    const char *packet =                           &rx_buf[  g_packet_buffers->rx.ptr + sizeof(*header)  ];

    // TODO: write packet to kernel packet queue
    (void)packet;

    g_packet_buffers->rx.ptr = math_ALIGN_UP(g_packet_buffers->rx.ptr + header->packet_size + sizeof(*header), sizeof(uint32_t));
    g_packet_buffers->rx.ptr %= RECV_BUFFER_SIZE_RAW;

    rtl_write(word, rtl8139_REG_CAPR, g_packet_buffers->rx.ptr - 0x10); // -0x10 for rtl8139 to be happy (CAPR is always 0x10 less than actual CAPR)
}

void rtl8139_interrupt_handler_impl()
{
    uint16_t status = rtl_read(word, rtl8139_REG_ISR);

    // We need to write the cause of the interrupt back to the reg.
    // Instead of having logic for each one, we just write the whole mask, as at
    // least one of the bits there is the cause
    rtl_write(word, rtl8139_REG_ISR, INTERRUPT_MASK);

    if (status & rtl8139_IMR_RECEIVE_OK)
    {
        rtl8139_recv_packer(status);
    }
    // TODO: implement for the following
    else if (status & rtl8139_IMR_RECEIVE_ERR)
    {
    }
    else if (status & rtl8139_IMR_TRANSMIT_OK)
    {
    }
    else if (status & rtl8139_IMR_TRANSMIT_ERR)
    {
    }

    defer({ pic_send_EOI(g_rtl.irq); });
}

void rtl8139_interrupt_handler()
{
    isr_IMPL_INTERRUPT(rtl8139_interrupt_handler_impl);
}

void rtl8139_register_interrupt_handler(uint8_t irq)
{
    assert(irq == pic_IRQ_FREE11 && "That's the only currently supported IRQ for the RTL8139");
    if (g_rtl.irq != irq)
    {
        pci_set_irq_number(g_rtl.address, irq);
        g_rtl.irq = irq;
    }

    assert(pci_get_irq_number(g_rtl.address) == irq);

    pic_clear_mask(irq);
    idt_register(pic_irq_number_to_idt(irq), IDT_gate_type_INTERRUPT, rtl8139_interrupt_handler);
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

    rs = rtl8139_init_recv_buffer();
    if (!IS_OK(rs))
    {
        return res_rtl8139_PACKETS_BUFFER_NO_MEMORY;
    }

    rtl8139_set_interrupt_mask(INTERRUPT_MASK);

    rtl8139_set_receive_config(rtl8139_RCR_ACCEPT_PHYSICAL_MATCH |
                               rtl8139_RCR_ACCEPT_BROADCAST | rtl8139_RCR_WRAP);

    rtl8139_enable_transmit_and_receive();

    return res_OK;
}
