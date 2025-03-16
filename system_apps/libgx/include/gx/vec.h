#pragma once

typedef struct {
    int x;
    int y;
} gx_Vec2;

typedef struct {
    double x;
    double y;
} gx_Vec2f;

/**
 * @brief Add two `Vec2`s
 */
gx_Vec2 gx_vec2_add(gx_Vec2 a, gx_Vec2 b);

/**
 * @brief Add two `Vec2f`s
 */
gx_Vec2f gx_vec2f_add(gx_Vec2f a, gx_Vec2f b);
