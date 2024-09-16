#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void put(char *str);
void putc(char ch);
void puts(char *str);

char *itoa(long value, char *str, size_t size, int base);
void printf(char *fmt, ...);

void io_clear_vga();

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
void get_string(char *buffer, size_t max_size);
