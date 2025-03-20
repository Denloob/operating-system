#pragma once

#include <stdint.h>

typedef struct __attribute__((packed)) {
    int16_t x;
    int16_t y;
    uint8_t buttons;
} gx_mouse_State;

int gx_mouse_init();

gx_mouse_State gx_mouse_get_state() __attribute__((warn_unused_result));
