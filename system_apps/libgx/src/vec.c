#include <gx/vec.h>

gx_Vec2 gx_vec2_add(gx_Vec2 a, gx_Vec2 b)
{
    return (gx_Vec2){
        .x = a.x+b.x,
        .y = a.y+b.y,
    };
}

gx_Vec2f gx_vec2f_add(gx_Vec2f a, gx_Vec2f b)
{
    return (gx_Vec2f){
        .x = a.x+b.x,
        .y = a.y+b.y,
    };
}
