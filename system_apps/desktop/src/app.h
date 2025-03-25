#pragma once

#include <gx/canvas.h>
#include <gx/vec.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    struct {
        gx_Vec2 pos;
        bool is_pressed;
        bool was_just_released;
    } mouse;

    struct {
        gx_Canvas *wallpaper;
    } assets;

    struct Button **buttons;
    size_t button_count;
    size_t button_capacity;
} App;
