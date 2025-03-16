#include <gx/canvas.h>
#include <gx/draw.h>
#include <gx/vec.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>

#define at2(x, y) buf[(canvas->width * (y)) + (x)]
#define at1(vec) at2((vec).x, (vec).y)

#define GET_MACRO(_1, _2, NAME, ...) NAME
#define at(...) GET_MACRO(__VA_ARGS__, at2, at1)(__VA_ARGS__)

void gx_draw_point(gx_Canvas *canvas, gx_Vec2 point, gx_Color color)
{
    int x = point.x;
    int y = point.y;
    if (x >= 0 && x < canvas->width && y >= 0 && y < canvas->height)
    {
        canvas->at(x, y) = color;
    }
}

void gx_draw_line(gx_Canvas *canvas, gx_Vec2 start, gx_Vec2 end, gx_Color color)
{
    if (start.x == end.x)
    {
        int x = start.x;
        int y0 = MIN(start.y, end.y);
        int y1 = MAX(start.y, end.y);
        for (int y = y0; y <= y1; y++)
        {
            gx_draw_point(canvas, (gx_Vec2){x, y}, color);
        }

        return;
    }

    if (start.y == end.y)
    {
        int y = start.y;
        int x0 = MIN(start.x, end.x);
        int x1 = MAX(start.x, end.x);
        for (int x = x0; x <= x1; x++)
        {
            gx_draw_point(canvas, (gx_Vec2){x, y}, color);
        }

        return;
    }

    int x0;
    int x1;
    int y0;
    int y1;
    if (start.x > end.x)
    {
        x0 = end.x;
        y0 = end.y;
        x1 = start.x;
        y1 = start.y;
    }
    else
    {
        x0 = start.x;
        y0 = start.y;
        x1 = end.x;
        y1 = end.y;
    }

    int dx = abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;

    int err = dx + dy;
    int e2 = 2;
    while (true)
    {
        gx_draw_point(canvas, (gx_Vec2){x0, y0}, color);
        if (x0 == x1 && y0 == y1)
        {
            break;
        }

        e2 = 2 * err;

        if (e2 >= dy)
        {
            err += dy;
            x0 += sx;
        }

        if (e2 <= dx)
        {
            err += dx;
            y0 += sy;
        }
    }
}

void gx_draw_rect(gx_Canvas *canvas, gx_Vec2 top_left, gx_Vec2 bottom_right,
                  gx_Color color)
{
    int left   = MAX(0,                  MIN(top_left.x, bottom_right.x));
    int right  = MIN(canvas->width - 1,  MAX(top_left.x, bottom_right.x));
    int top    = MAX(0,                  MIN(top_left.y, bottom_right.y));
    int bottom = MIN(canvas->height - 1, MAX(top_left.y, bottom_right.y));

    gx_draw_line(canvas, (gx_Vec2){left, top},     (gx_Vec2){right, top},       color);
    gx_draw_line(canvas, (gx_Vec2){left, bottom},  (gx_Vec2){right, bottom},    color);
    gx_draw_line(canvas, (gx_Vec2){left, top},     (gx_Vec2){left, bottom},     color);
    gx_draw_line(canvas, (gx_Vec2){right, top},    (gx_Vec2){right, bottom},    color);
}

void gx_draw_fill_rect(gx_Canvas *canvas, gx_Vec2 top_left, gx_Vec2 bottom_right,
                       gx_Color color)
{
    int left   = MAX(0,                  MIN(top_left.x, bottom_right.x));
    int right  = MIN(canvas->width - 1,  MAX(top_left.x, bottom_right.x));
    int top    = MAX(0,                  MIN(top_left.y, bottom_right.y));
    int bottom = MIN(canvas->height - 1, MAX(top_left.y, bottom_right.y));

    for (int y = top; y <= bottom; y++)
    {
        for (int x = left; x <= right; x++)
        {
            gx_draw_point(canvas, (gx_Vec2){x, y}, color);
        }
    }
}

void gx_draw_circle(gx_Canvas *canvas, gx_Vec2 center, int radius, gx_Color color)
{
}

void gx_draw_fill_circle(gx_Canvas *canvas, gx_Vec2 center, int radius,
                         gx_Color color)
{
}
