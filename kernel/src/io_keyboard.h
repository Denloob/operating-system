#pragma once

#include "compiler_macros.h"
#include "res.h"
#include <stdbool.h>
#include <stdint.h>

#define IO_KEY_MODIFIER_SHIFT (1 << 0)

typedef struct {
    uint8_t code;
    uint8_t modifiers;
} io_Key;

io_Key io_keyboard_wait_key();

#define res_SELF_TEST_FAILED "Keyboard self test failed"
res io_keyboard_reset_and_self_test() WUR;

// Check if a keyboard key was pressed and ready to be read.
//  When it is, io_keyboard_wait_key will return without blocking.
bool io_keyboard_is_key_ready() WUR;

__attribute__((always_inline)) inline
static int io_keyboard_wait_key_code()
{
    return io_keyboard_wait_key().code;
}

char io_keyboard_key_to_char(io_Key key);

#define IO_KEYCODE_RELEASE_BITMASK 0x80

#define IO_KEYCODE_ESC          0x01
#define IO_KEYCODE_1            0x02
#define IO_KEYCODE_2            0x03
#define IO_KEYCODE_3            0x04
#define IO_KEYCODE_4            0x05
#define IO_KEYCODE_5            0x06
#define IO_KEYCODE_6            0x07
#define IO_KEYCODE_7            0x08
#define IO_KEYCODE_8            0x09
#define IO_KEYCODE_9            0x0A
#define IO_KEYCODE_0            0x0B
#define IO_KEYCODE_MINUS        0x0C
#define IO_KEYCODE_EQUALS       0x0D
#define IO_KEYCODE_BACKSPACE    0x0E
#define IO_KEYCODE_TAB          0x0F
#define IO_KEYCODE_Q            0x10
#define IO_KEYCODE_W            0x11
#define IO_KEYCODE_E            0x12
#define IO_KEYCODE_R            0x13
#define IO_KEYCODE_T            0x14
#define IO_KEYCODE_Y            0x15
#define IO_KEYCODE_U            0x16
#define IO_KEYCODE_I            0x17
#define IO_KEYCODE_O            0x18
#define IO_KEYCODE_P            0x19
#define IO_KEYCODE_LEFTBRACKET  0x1A
#define IO_KEYCODE_RIGHTBRACKET 0x1B
#define IO_KEYCODE_ENTER        0x1C
#define IO_KEYCODE_LCTRL        0x1D
#define IO_KEYCODE_A            0x1E
#define IO_KEYCODE_S            0x1F
#define IO_KEYCODE_D            0x20
#define IO_KEYCODE_F            0x21
#define IO_KEYCODE_G            0x22
#define IO_KEYCODE_H            0x23
#define IO_KEYCODE_J            0x24
#define IO_KEYCODE_K            0x25
#define IO_KEYCODE_L            0x26
#define IO_KEYCODE_SEMICOLON    0x27
#define IO_KEYCODE_QUOTE        0x28
#define IO_KEYCODE_BACKTICK     0x29
#define IO_KEYCODE_LSHIFT       0x2A
#define IO_KEYCODE_BACKSLASH    0x2B
#define IO_KEYCODE_Z            0x2C
#define IO_KEYCODE_X            0x2D
#define IO_KEYCODE_C            0x2E
#define IO_KEYCODE_V            0x2F
#define IO_KEYCODE_B            0x30
#define IO_KEYCODE_N            0x31
#define IO_KEYCODE_M            0x32
#define IO_KEYCODE_COMMA        0x33
#define IO_KEYCODE_PERIOD       0x34
#define IO_KEYCODE_SLASH        0x35
#define IO_KEYCODE_RSHIFT       0x36
#define IO_KEYCODE_LEFT_ALT     0x38
#define IO_KEYCODE_SPACE        0x39
#define IO_KEYCODE_CAPSLOCK     0x3A
#define IO_KEYCODE_F1           0x3B
#define IO_KEYCODE_F2           0x3C
#define IO_KEYCODE_F3           0x3D
#define IO_KEYCODE_F4           0x3E
#define IO_KEYCODE_F5           0x3F
#define IO_KEYCODE_F6           0x40
#define IO_KEYCODE_F7           0x41
#define IO_KEYCODE_F8           0x42
#define IO_KEYCODE_F9           0x43
#define IO_KEYCODE_F10          0x44
#define IO_KEYCODE_ARROW_LEFT   0x4b
#define IO_KEYCODE_ARROW_RIGHT  0x4d
#define IO_KEYCODE_ARROW_UP     0x48
#define IO_KEYCODE_ARROW_DOWN   0x50

#define IO_CHAR_ARROW_LEFT      0x1b
#define IO_CHAR_ARROW_RIGHT     0x1a
#define IO_CHAR_ARROW_UP        0x18
#define IO_CHAR_ARROW_DOWN      0x19
