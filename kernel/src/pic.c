#include "io.h"
#include "pic.h"
#include <stdint.h>

#define PIC_EOI 0x20 /* End-of-interrupt */
#define PIC1 0x20    /* IO base address for master PIC */
#define PIC2 0xA0    /* IO base address for slave PIC */
#define PIC1_COMMAND PIC1
#define PIC1_DATA (PIC1 + 1)
#define PIC2_COMMAND PIC2
#define PIC2_DATA (PIC2 + 1)

void pic_send_EOI(pic_IRQ interrupt_request)
{
#define PIC1_REQUEST_PIN_COUNT 8
    if (interrupt_request >= PIC1_REQUEST_PIN_COUNT)
        out_byte(PIC2_COMMAND, PIC_EOI);

    out_byte(PIC1_COMMAND, PIC_EOI);
}

/* reinitialize the PIC controllers, giving them specified vector offsets
   rather than 8h and 70h, as configured by default */

// Interrupt command words
#define ICW1_ICW4 0x01      /* Indicates that ICW4 will be present */
#define ICW1_SINGLE 0x02    /* Single (cascade) mode */
#define ICW1_INTERVAL4 0x04 /* Call address interval 4 (8) */
#define ICW1_LEVEL 0x08     /* Level triggered (edge) mode */
#define ICW1_INIT 0x10      /* Initialization - required! */

#define ICW3_SLAVE_AT_IRQ2 0b100
#define ICW3_SET_CASCADE_IDENTITY pic_IRQ__CASCADE // Tells the slave its cascade identity

#define ICW4_8086 0x01       /* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO 0x02       /* Auto (normal) EOI */
#define ICW4_BUF_SLAVE 0x08  /* Buffered mode/slave */
#define ICW4_BUF_MASTER 0x0C /* Buffered mode/master */
#define ICW4_SFNM 0x10       /* Special fully nested (not) */

void PIC_remap(int offset1, int offset2)
{
    // TODO: add io_wait after each out_byte

    uint8_t mask1 = in_byte(PIC1_DATA); // save masks
    uint8_t mask2 = in_byte(PIC2_DATA);

    out_byte(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4); // starts the initialization sequence (in cascade mode)
    out_byte(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    out_byte(PIC1_DATA, offset1); // ICW2: Master PIC vector offset
    out_byte(PIC2_DATA, offset2); // ICW2: Slave PIC vector offset
    out_byte(PIC1_DATA, ICW3_SLAVE_AT_IRQ2); // ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
    out_byte(PIC2_DATA, ICW3_SET_CASCADE_IDENTITY); // ICW3: tell Slave PIC its cascade identity (0000 0010)

    out_byte(PIC1_DATA, ICW4_8086); // ICW4: have the PICs use 8086 mode (and not 8080 mode)
    out_byte(PIC2_DATA, ICW4_8086);

    out_byte(PIC1_DATA, mask1); // restore saved masks.
    out_byte(PIC2_DATA, mask2);
}

void pic_mask_all()
{
    out_byte(PIC1_DATA, 0xff);
    out_byte(PIC2_DATA, 0xff);
}

void pic_set_mask(pic_IRQ interrupt_request)
{
    uint16_t port;

    if (interrupt_request < 8)
    {
        port = PIC1_DATA;
    }
    else
    {
        port = PIC2_DATA;
        interrupt_request -= 8;
    }
    uint16_t value = in_byte(port) | (1 << interrupt_request);
    out_byte(port, value);
}

void pic_clear_mask(pic_IRQ interrupt_request)
{
    uint16_t port;

    if (interrupt_request < 8)
    {
        port = PIC1_DATA;
    }
    else
    {
        port = PIC2_DATA;
        interrupt_request -= 8;
    }
    uint16_t value = in_byte(port) & ~(1 << interrupt_request);
    out_byte(port, value);
}
