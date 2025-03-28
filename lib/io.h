#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "printf.h"

void put(const char *str);
void putc(char ch);
#define putchar putc
void puts(const char *str);

static inline uint64_t get_cpu_flags()
{
    uint64_t flags;
    asm (
        "pushf\n"
        "pop %0\n"
        : "=r"(flags)
        :
        : "memory"
    );

    return flags;
}

static inline void set_cpu_flags(uint64_t flags)
{
    asm (
        "push %0\n"
        "popf\n"
        :
        : "r"(flags)
        : "memory", "cc"
    );
}

static inline void sti()
{
    asm volatile ("sti" : : : "memory");
}

static inline void cli()
{
    asm volatile ("cli" : : : "memory");
}

static inline void hlt()
{
    asm volatile ("hlt" : : : "memory");
}

 __attribute__((noreturn)) static inline void cli_hlt()
{
    asm volatile ("cli; hlt" : : : "memory");
    __builtin_unreachable();
}

static inline void out_byte(uint16_t port, uint8_t val)
{
    __asm__ volatile("out %1, %0" : : "a"(val), "Nd"(port) : "memory");
}

static inline void out_word(uint16_t port, uint16_t val)
{
    __asm__ volatile("out %1, %0" : : "a"(val), "Nd"(port) : "memory");
}

static inline void out_dword(uint16_t port, uint32_t val)
{
    __asm__ volatile("out %1, %0" : : "a"(val), "Nd"(port) : "memory");
}

static inline uint8_t in_byte(uint16_t port)
{
    uint8_t ret;
    __asm__ volatile("in %0, %1" : "=a"(ret) : "Nd"(port) : "memory");
    return ret;
}

static inline uint16_t in_word(uint16_t port)
{
    uint16_t ret=0;
    __asm__ volatile("in %0, %1" : "=a"(ret) : "Nd"(port) : "memory");
    return ret;
}

static inline uint32_t in_dword(uint16_t port)
{
    uint32_t ret = 0;
    __asm__ volatile("in %0, %1" : "=a"(ret) : "Nd"(port) : "memory");
    return ret;
}

void repeated_in_dword(uint16_t port , uint32_t *out , uint64_t count);

void io_clear_vga();

int io_wait_key_raw();

extern int (*io_input_keyboard_key)();

extern volatile uint8_t *io_vga_addr_base;

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

// An upside down question mark in our font
#define IO_KEY_UNKNOWN (uint8_t)0xa8

/**
 * @brief Converts a (set 1) keycode to an ascii character. If there's no ascii
 *          character for the given keycode, IO_KEY_UNKNOWN is returned.
 *
 * @param keycode
 * @return Corresponding ascii character or IO_KEY_UNKNOWN.
 */
char key_to_char(int keycode);

/* Function to get a string from the keyboard and write it into the provided array
  input: pointer to array , array max size
*/
void get_string(char *buffer, size_t max_size);

/**
 * @brief - the funciton adds a small delay
 */
void io_delay();
