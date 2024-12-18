#pragma once

/*
 * NOTE: This is not drivers for character device support in general.
 *          For that, see `char_device.h` and its `.c`.
*        This is specifically for the special character devices such as `null`,
*           `zero`, `pts`, etc.
*/


/* The minor numbers for the char special devices */
typedef enum {
    char_special_device_MINOR_NULL,
    char_special_device_MINOR_ZERO,
    char_special_device_MINOR_TTY,
} char_special_device_MinorDeviceType;

#define char_special_device_MAJOR_NUMBER 1

/**
 * @brief Initialize and register the character special device.
 */
void char_special_device_init();
