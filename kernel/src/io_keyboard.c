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
    bool shift = key.modifiers & IO_KEY_MODIFIER_SHIFT;

    switch (key.code)
    {
        default:                     return IO_KEY_UNKNOWN;
        case IO_KEYCODE_1:           return shift ? '!' : '1';
        case IO_KEYCODE_2:           return shift ? '@' : '2';
        case IO_KEYCODE_3:           return shift ? '#' : '3';
        case IO_KEYCODE_4:           return shift ? '$' : '4';
        case IO_KEYCODE_5:           return shift ? '%' : '5';
        case IO_KEYCODE_6:           return shift ? '^' : '6';
        case IO_KEYCODE_7:           return shift ? '&' : '7';
        case IO_KEYCODE_8:           return shift ? '*' : '8';
        case IO_KEYCODE_9:           return shift ? '(' : '9';
        case IO_KEYCODE_0:           return shift ? ')' : '0';
        case IO_KEYCODE_MINUS:       return shift ? '_' : '-';
        case IO_KEYCODE_EQUALS:      return shift ? '+' : '=';
        case IO_KEYCODE_BACKSPACE:   return '\b';
        case IO_KEYCODE_SPACE:       return ' ';
        case IO_KEYCODE_TAB:         return '\t';
        case IO_KEYCODE_Q:           return shift ? 'Q' : 'q';
        case IO_KEYCODE_W:           return shift ? 'W' : 'w';
        case IO_KEYCODE_E:           return shift ? 'E' : 'e';
        case IO_KEYCODE_R:           return shift ? 'R' : 'r';
        case IO_KEYCODE_T:           return shift ? 'T' : 't';
        case IO_KEYCODE_Y:           return shift ? 'Y' : 'y';
        case IO_KEYCODE_U:           return shift ? 'U' : 'u';
        case IO_KEYCODE_I:           return shift ? 'I' : 'i';
        case IO_KEYCODE_O:           return shift ? 'O' : 'o';
        case IO_KEYCODE_P:           return shift ? 'P' : 'p';
        case IO_KEYCODE_LEFTBRACKET: return shift ? '{' : '[';
        case IO_KEYCODE_RIGHTBRACKET:return shift ? '}' : ']';
        case IO_KEYCODE_ENTER:       return '\n';
        case IO_KEYCODE_A:           return shift ? 'A' : 'a';
        case IO_KEYCODE_S:           return shift ? 'S' : 's';
        case IO_KEYCODE_D:           return shift ? 'D' : 'd';
        case IO_KEYCODE_F:           return shift ? 'F' : 'f';
        case IO_KEYCODE_G:           return shift ? 'G' : 'g';
        case IO_KEYCODE_H:           return shift ? 'H' : 'h';
        case IO_KEYCODE_J:           return shift ? 'J' : 'j';
        case IO_KEYCODE_K:           return shift ? 'K' : 'k';
        case IO_KEYCODE_L:           return shift ? 'L' : 'l';
        case IO_KEYCODE_SEMICOLON:   return shift ? ':' : ';';
        case IO_KEYCODE_QUOTE:       return shift ? '"' : '\'';
        case IO_KEYCODE_BACKTICK:    return shift ? '~' : '`';
        case IO_KEYCODE_BACKSLASH:   return shift ? '|' : '\\';
        case IO_KEYCODE_Z:           return shift ? 'Z' : 'z';
        case IO_KEYCODE_X:           return shift ? 'X' : 'x';
        case IO_KEYCODE_C:           return shift ? 'C' : 'c';
        case IO_KEYCODE_V:           return shift ? 'V' : 'v';
        case IO_KEYCODE_B:           return shift ? 'B' : 'b';
        case IO_KEYCODE_N:           return shift ? 'N' : 'n';
        case IO_KEYCODE_M:           return shift ? 'M' : 'm';
        case IO_KEYCODE_COMMA:       return shift ? '<' : ',';
        case IO_KEYCODE_PERIOD:      return shift ? '>' : '.';
        case IO_KEYCODE_SLASH:       return shift ? '?' : '/';
    }
}
