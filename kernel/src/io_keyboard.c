#include "io.isr.h"
#include "io_keyboard.h"
#include "io.h"
#include "res.h"
#include <stdbool.h>

bool io_keyboard_is_key_ready()
{
    bool is_ringbuffer_empty = io_keyboard_buffer_head == io_keyboard_buffer_tail;

    return !is_ringbuffer_empty;
}

io_Key io_keyboard_wait_key()
{
    bool is_ringbuffer_empty;
    do
    {
        // NOTE: here we do not disable interrupt flag, because if we are interrupted in the middle of the compare by the keyboard handler (which can only add to the buffer, not remove), best case it's just before we load vars, and we get the var to `true`. Worst case, it's interrupted when we already read the memory, and it will be `false` for one more round. So no need for `sti` and `cli`.
        is_ringbuffer_empty = io_keyboard_buffer_head == io_keyboard_buffer_tail;
    } while (is_ringbuffer_empty);

    cli();
    io_Key key = io_keyboard_buffer[io_keyboard_buffer_tail];
    io_keyboard_buffer_tail++;
    io_keyboard_buffer_tail %= IO_KEYBOARD_BUFFER_SIZE;
    sti();

    return key;
}

res io_keyboard_reset_and_self_test()
{
#define KEYBOARD_PORT 0x60

#define RESEND 0xFE
#define SELF_TEST_PASSED 0xAA

#define PERFORM_SELF_TEST 0xFF

    uint8_t response;
    do
    {
        out_byte(KEYBOARD_PORT, PERFORM_SELF_TEST);
        io_delay();
    } while ((response = in_byte(KEYBOARD_PORT)) == RESEND);

    const uint8_t test_result = in_byte(KEYBOARD_PORT);
    if (test_result == SELF_TEST_PASSED)
        return res_OK;

    return res_SELF_TEST_FAILED;
}

char io_keyboard_key_to_char(io_Key key)
{
    static const char no_shift[] = {
        [IO_KEYCODE_1] = '1', [IO_KEYCODE_2] = '2', [IO_KEYCODE_3] = '3',
        [IO_KEYCODE_4] = '4', [IO_KEYCODE_5] = '5', [IO_KEYCODE_6] = '6',
        [IO_KEYCODE_7] = '7', [IO_KEYCODE_8] = '8', [IO_KEYCODE_9] = '9',
        [IO_KEYCODE_0] = '0', [IO_KEYCODE_MINUS] = '-', [IO_KEYCODE_EQUALS] = '=',
        [IO_KEYCODE_BACKSPACE] = '\b', [IO_KEYCODE_SPACE] = ' ', [IO_KEYCODE_TAB] = '\t',
        [IO_KEYCODE_Q] = 'q', [IO_KEYCODE_W] = 'w', [IO_KEYCODE_E] = 'e',
        [IO_KEYCODE_R] = 'r', [IO_KEYCODE_T] = 't', [IO_KEYCODE_Y] = 'y',
        [IO_KEYCODE_U] = 'u', [IO_KEYCODE_I] = 'i', [IO_KEYCODE_O] = 'o',
        [IO_KEYCODE_P] = 'p', [IO_KEYCODE_LEFTBRACKET] = '[', [IO_KEYCODE_RIGHTBRACKET] = ']',
        [IO_KEYCODE_ENTER] = '\n', [IO_KEYCODE_A] = 'a', [IO_KEYCODE_S] = 's',
        [IO_KEYCODE_D] = 'd', [IO_KEYCODE_F] = 'f', [IO_KEYCODE_G] = 'g',
        [IO_KEYCODE_H] = 'h', [IO_KEYCODE_J] = 'j', [IO_KEYCODE_K] = 'k',
        [IO_KEYCODE_L] = 'l', [IO_KEYCODE_SEMICOLON] = ';', [IO_KEYCODE_QUOTE] = '\'',
        [IO_KEYCODE_BACKTICK] = '`', [IO_KEYCODE_BACKSLASH] = '\\', [IO_KEYCODE_Z] = 'z',
        [IO_KEYCODE_X] = 'x', [IO_KEYCODE_V] = 'v', [IO_KEYCODE_C] = 'c',
        [IO_KEYCODE_B] = 'b', [IO_KEYCODE_N] = 'n', [IO_KEYCODE_M] = 'm',
        [IO_KEYCODE_COMMA] = ',', [IO_KEYCODE_PERIOD] = '.', [IO_KEYCODE_SLASH] = '/',
    };

    static const char with_shift[] = {
        [IO_KEYCODE_1] = '!', [IO_KEYCODE_2] = '@', [IO_KEYCODE_3] = '#',
        [IO_KEYCODE_4] = '$', [IO_KEYCODE_5] = '%', [IO_KEYCODE_6] = '^',
        [IO_KEYCODE_7] = '&', [IO_KEYCODE_8] = '*', [IO_KEYCODE_9] = '(',
        [IO_KEYCODE_0] = ')', [IO_KEYCODE_MINUS] = '_', [IO_KEYCODE_EQUALS] = '+',
        [IO_KEYCODE_BACKSPACE] = '\b', [IO_KEYCODE_SPACE] = ' ', [IO_KEYCODE_TAB] = '\t',
        [IO_KEYCODE_Q] = 'Q', [IO_KEYCODE_W] = 'W', [IO_KEYCODE_E] = 'E',
        [IO_KEYCODE_R] = 'R', [IO_KEYCODE_T] = 'T', [IO_KEYCODE_Y] = 'Y',
        [IO_KEYCODE_U] = 'U', [IO_KEYCODE_I] = 'I', [IO_KEYCODE_O] = 'O',
        [IO_KEYCODE_P] = 'P', [IO_KEYCODE_LEFTBRACKET] = '{', [IO_KEYCODE_RIGHTBRACKET] = '}',
        [IO_KEYCODE_ENTER] = '\n', [IO_KEYCODE_A] = 'A', [IO_KEYCODE_S] = 'S',
        [IO_KEYCODE_D] = 'D', [IO_KEYCODE_F] = 'F', [IO_KEYCODE_G] = 'G',
        [IO_KEYCODE_H] = 'H', [IO_KEYCODE_J] = 'J', [IO_KEYCODE_K] = 'K',
        [IO_KEYCODE_L] = 'L', [IO_KEYCODE_SEMICOLON] = ':', [IO_KEYCODE_QUOTE] = '"',
        [IO_KEYCODE_BACKTICK] = '~', [IO_KEYCODE_BACKSLASH] = '|', [IO_KEYCODE_Z] = 'Z',
        [IO_KEYCODE_X] = 'X', [IO_KEYCODE_V] = 'V', [IO_KEYCODE_C] = 'C',
        [IO_KEYCODE_B] = 'B', [IO_KEYCODE_N] = 'N', [IO_KEYCODE_M] = 'M',
        [IO_KEYCODE_COMMA] = '<', [IO_KEYCODE_PERIOD] = '>', [IO_KEYCODE_SLASH] = '?',
    };

    const bool shift = key.modifiers & IO_KEY_MODIFIER_SHIFT;

    _Static_assert(sizeof(with_shift) == sizeof(no_shift));
    if (key.code > sizeof(with_shift))
    {
        return IO_KEY_UNKNOWN;
    }

    const char result = shift ? with_shift[key.code] : no_shift[key.code];
    return result ? result : IO_KEY_UNKNOWN;
}
