#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

void put(char *str);
void putc(char ch);
void puts(char *str);

static inline void out_byte(uint16_t port, uint8_t val)
{
    __asm__ volatile("out %1, %0" : : "a"(val), "Nd"(port) : "memory");
}

static inline void out_word(uint16_t port, uint16_t val)
{
    __asm__ volatile("out %1, %0" : : "a"(val), "Nd"(port) : "memory");
}

static inline uint8_t in_byte(uint16_t port)
{
    uint8_t ret;
    __asm__ volatile("in %0, %1" : "=a"(ret) : "Nd"(port) : "memory");
    return ret;
}

static inline void out_dword(uint16_t port, uint32_t val)
{
    out_byte(port, (val & 0xFF));
    out_byte(port, (val >> 8) & 0xFF);
    out_byte(port, (val >> 16) & 0xFF);
    out_byte(port, (val >> 24) & 0xFF);
}

static inline uint32_t in_dword(uint16_t port) 
{
    uint32_t ret = 0;
    ret |= in_byte(port);
    ret |= in_byte(port) << 8;
    ret |= in_byte(port) << 16;
    ret |= in_byte(port) << 24;
    return ret;
}




char *itoa(int value, char *str, size_t size, int base);
char *ltoa(long value, char *str, size_t size, int base);
char *lltoa(long long value, char *str, size_t size, int base);
void printf(char *fmt, ...);

void io_clear_vga();

int wait_key();

typedef enum KeycodeType
{
    KCT_IS_PRESS = 0x1,
} KeycodeType;

/**
 * @brief Gets the type of the given keycode.
 *
 * @param keycode The keycode to get the type of
 * @return Bitmap of KeycodeType
 * @see KeycodeType
 */
int get_key_type(int keycode);

/**
 * @brief Converts a (set 1) keycode to an ascii character. If there's no ascii
 *          character for the given keycode, '\0' is returned.
 *
 * @param keycode
 * @return Corresponding ascii character or '\0'
 */
char key_to_char(int keycode);
/* Function to get a string from the keyboard and write it into the provided array
  input: pointer to array , array max size
*/
void get_string(char *buffer, size_t max_size);

