#include "button.h"
#include <assert.h>
#include <stdlib.h>

// NOTE: gcc produces a more optimized asm for our use case when using early returns compared
//          to chaining `&&`. Pretty surprising though, I believed it would boil down the problem to the same asm.
static bool button_is_pos_inside(Button *button, gx_Vec2 pos)
{
    if (pos.x < button->pos.x || pos.y < button->pos.y)
        return false;

    if (pos.x > button->pos.x + button->size.x || pos.y > button->pos.y + button->size.y)
        return false;

    return true;
}

void button_update(Button *button, App *app)
{
    assert(button->callback != NULL);

    if (!app->mouse.was_just_released || !button_is_pos_inside(button, app->mouse.pos))
    {
        return;
    }

    button->callback(button->callback_arg);
}
