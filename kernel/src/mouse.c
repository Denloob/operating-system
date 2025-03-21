#include "assert.h"
#include "smartptr.h"
#include "isr.h"
#include "io.h"
#include "pic.h"
#include "vga.h"

#define PS2_PORT_DATA 0x60
#define PS2_PORT_COMMAND 0x64

#define PS2_READ_CONFIG_COMMAND 0x20
#define PS2_WRITE_CONFIG_COMMAND 0x60
#define PS2_CONFIG_MOUSE_IRQ_ENABLE 0x02 // Bit 1: Enable IRQ12 for mouse
#define MOUSE_ENABLE_COMMAND 0xA8
#define MOUSE_RESET_COMMAND 0xFF
#define MOUSE_SET_DEFAULTS_COMMAND 0xF6
#define MOUSE_ENABLE_REPORTING_COMMAND 0xF4
#define PS2_NEXT_DATA_FOR_MOUSE_COMMAND 0xD4

#define MOUSE_RESP_ACK 0xFA
#define MOUSE_RESP_SELF_TEST_OK 0xAA
#define MOUSE_RESP_DEVICE_ID_0 0x00

static struct
{
    int16_t x;
    int16_t y;
    uint8_t buttons;
    uint8_t packet[3];
    uint8_t byte_count;
} g_mouse_state = {0};

/**
 * @brief Sends a command or data byte to the PS/2 mouse via the controller.
 * @param data The byte to send to the mouse.
 */
static void send_to_mouse(uint8_t data)
{
    out_byte(PS2_PORT_COMMAND, PS2_NEXT_DATA_FOR_MOUSE_COMMAND);
    io_delay();
    out_byte(PS2_PORT_DATA, data);
    io_delay();
}

/**
 * @brief Receives a response byte from the PS/2 mouse.
 * @return The byte read from the mouse.
 */
static uint8_t recv_from_mouse()
{
    while ((in_byte(PS2_PORT_COMMAND) & 1) == 0)
    {
    }
    return in_byte(PS2_PORT_DATA);
}

/**
 * @brief Flushes the PS/2 output buffer to clear stale data.
 */
static void flush_ps2_buffer()
{
    while (in_byte(PS2_PORT_COMMAND) & 1)
    {
        in_byte(PS2_PORT_DATA);
    }
}


void mouse_init()
{
    flush_ps2_buffer();
    out_byte(PS2_PORT_COMMAND, MOUSE_ENABLE_COMMAND);
    io_delay();

    send_to_mouse(MOUSE_RESET_COMMAND);
    assert(recv_from_mouse() == MOUSE_RESP_ACK);
    assert(recv_from_mouse() == MOUSE_RESP_SELF_TEST_OK);
    assert(recv_from_mouse() == MOUSE_RESP_DEVICE_ID_0);

    send_to_mouse(MOUSE_SET_DEFAULTS_COMMAND);
    assert(recv_from_mouse() == MOUSE_RESP_ACK);

    send_to_mouse(MOUSE_ENABLE_REPORTING_COMMAND);
    assert(recv_from_mouse() == MOUSE_RESP_ACK);

    out_byte(PS2_PORT_COMMAND, PS2_READ_CONFIG_COMMAND);
    io_delay();
    while ((in_byte(PS2_PORT_COMMAND) & 1) == 0)
    {
    }
    uint8_t config = in_byte(PS2_PORT_DATA);
    config |= PS2_CONFIG_MOUSE_IRQ_ENABLE;
    out_byte(PS2_PORT_COMMAND, PS2_WRITE_CONFIG_COMMAND);
    io_delay();
    out_byte(PS2_PORT_DATA, config);
    io_delay();

    g_mouse_state.x = 0;
    g_mouse_state.y = 0;
    g_mouse_state.buttons = 0;
    g_mouse_state.byte_count = 0;
}

static void update_mouse_pos(int x_delta, int y_delta)
{
    g_mouse_state.x += x_delta;
    g_mouse_state.y -= y_delta;

    if (g_mouse_state.x < 0) g_mouse_state.x = 0;
    if (g_mouse_state.x > VGA_GRAPHICS_WIDTH) g_mouse_state.x = VGA_GRAPHICS_WIDTH;
    if (g_mouse_state.y < 0) g_mouse_state.y = 0;
    if (g_mouse_state.y > VGA_GRAPHICS_HEIGHT) g_mouse_state.y = VGA_GRAPHICS_HEIGHT;
}

static void __attribute__((used, sysv_abi)) mouse_isr_impl(isr_InterruptFrame *frame)
{
    defer({ pic_send_EOI(pic_IRQ_PS2_MOUSE); });

    uint8_t data = in_byte(PS2_PORT_DATA);
    g_mouse_state.packet[g_mouse_state.byte_count++] = data;

    if (g_mouse_state.byte_count == 3 && (g_mouse_state.packet[0] & 0x8))
    {
        g_mouse_state.buttons = g_mouse_state.packet[0] & 0x7;

        // 9-bit signed deltas - subtract 256 if sign bit set
        int16_t x_delta = g_mouse_state.packet[1] - ((g_mouse_state.packet[0] << 4) & 0x100);
        int16_t y_delta = g_mouse_state.packet[2] - ((g_mouse_state.packet[0] << 3) & 0x100);

        update_mouse_pos(x_delta, y_delta);

        g_mouse_state.byte_count = 0;

    }
}

void __attribute__((naked)) mouse_isr()
{
    isr_IMPL_INTERRUPT(mouse_isr_impl);
}


int16_t mouse_get_x()
{
    return g_mouse_state.x;
}


int16_t mouse_get_y()
{
    return g_mouse_state.y;
}


uint8_t mouse_get_buttons()
{
    return g_mouse_state.buttons;
}
