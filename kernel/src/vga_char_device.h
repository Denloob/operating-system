#pragma once

typedef enum {
    vga_char_device_MINOR_SCREEN,
    vga_char_device_MINOR_COLOR_PALETTE,
    vga_char_device_MINOR_CONFIG,
} vga_char_device_MinorDeviceType;

enum {
    vga_char_device_CONFIG_TEXT     = '0',
    vga_char_device_CONFIG_GRAPHICS = '1',
};

#define vga_char_device_MAJOR_NUMBER 2

/**
 * @brief Initialize and register the VGA special device.
 */
void vga_char_device_init();
