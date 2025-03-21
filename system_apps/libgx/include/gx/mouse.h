#pragma once

#include <stdint.h>

typedef struct __attribute__((packed)) {
    int16_t x;
    int16_t y;
    uint8_t buttons;
} gx_mouse_State;

int gx_mouse_init();

#define gx_mouse_BUTTON_LEFT   (1 << 0)
#define gx_mouse_BUTTON_RIGHT  (1 << 1)
#define gx_mouse_BUTTON_MIDDLE (1 << 2)

gx_mouse_State gx_mouse_get_state() __attribute__((warn_unused_result));
