#pragma once

typedef enum {
    mouse_char_device_MINOR_MOUSE,
} mouse_char_device_MinorDeviceType;

#define mouse_char_device_MAJOR_NUMBER 3

/**
 * @brief Initialize and register the mouse special device.
 */
void mouse_char_device_init();
