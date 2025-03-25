#pragma once

#include <gx/vec.h>
#include <gx/canvas.h>
#include "app.h"

typedef void (*button_Callback)(void *arg);

typedef struct Button {
    gx_Vec2 pos;
    gx_Vec2 size;
    gx_Canvas *texture;

    void *callback_arg;
    button_Callback callback;
} Button;

/**
 * @brief Updates the given button. If it was pressed, will call the callback
 *          with its argument.
 */
void button_update(Button *button, App *app);
