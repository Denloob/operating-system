#include <stdint.h>
#include <math.h>
#include <gx/draw.h>
#include <gx/vec.h>
#include <gx/gx.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>

void die(const char *message)
{
    gx_deinit();

    printf("tetris: %s\n", message);
    exit(1);
}

gx_Canvas *g_canvas;

#define COLOR_WHITE 7
#define COLOR_BLACK 0
#define SPEED 1000

#define SQUARE_SIZE 10

int game_board_begin;
int game_board_end;

void shape_square(gx_Canvas *canvas, gx_Vec2 pos)
{
    gx_draw_fill_rect_wh(canvas, pos, SQUARE_SIZE, SQUARE_SIZE, 0x02);
    gx_draw_rect_wh(canvas, pos, SQUARE_SIZE, SQUARE_SIZE, 0x10);
}

void shape_L(gx_Canvas *canvas, gx_Vec2 pos)
{
    shape_square(canvas, gx_vec2_add(pos, (gx_Vec2){ 0, 0 }));
    shape_square(canvas, gx_vec2_add(pos, (gx_Vec2){ 0, SQUARE_SIZE }));
    shape_square(canvas, gx_vec2_add(pos, (gx_Vec2){ 0, 2 * SQUARE_SIZE }));
    shape_square(canvas, gx_vec2_add(pos, (gx_Vec2){ 0, 3 * SQUARE_SIZE }));
    shape_square(canvas, gx_vec2_add(pos, (gx_Vec2){ SQUARE_SIZE, 3 * SQUARE_SIZE }));
}

void draw_grid(gx_Canvas *canvas)
{
    for (int i = SQUARE_SIZE - 1; i < g_canvas->height; i += SQUARE_SIZE)
    {
        gx_draw_line(g_canvas, (gx_Vec2){0, i }, (gx_Vec2){g_canvas->width, i }, 7);
    }

    for (int i = game_board_begin - 1; i < game_board_end; i += SQUARE_SIZE)
    {
        gx_draw_line(g_canvas, (gx_Vec2){i , 0}, (gx_Vec2){i , g_canvas->height}, 7);
    }
}

int main(int argc, char **argv)
{
    int ret = gx_init();
    if (ret < 0) die("gx_init: couldn't initialize gx");

    g_canvas = gx_canvas_create();
    if (g_canvas == NULL) die("gx_canvas_create: out of memory");

    game_board_begin = g_canvas->width / 3;
    game_board_end = g_canvas->width * 2 / 3;

    const int bg_color = 20;
    while (1)
    {
        memset(g_canvas->buf, 0, g_canvas->width * g_canvas->height);
        gx_draw_fill_rect_wh(g_canvas, (gx_Vec2){0,0}, g_canvas->width / 3, g_canvas->height, bg_color);
        gx_draw_fill_rect_wh(g_canvas, (gx_Vec2){g_canvas->width * 2 / 3,0}, g_canvas->width, g_canvas->height, bg_color);

        shape_L(g_canvas, (gx_Vec2){g_canvas->width / 3 + SQUARE_SIZE, SQUARE_SIZE});

        shape_L(g_canvas, (gx_Vec2){g_canvas->width / 3, SQUARE_SIZE*3});
        shape_L(g_canvas, (gx_Vec2){g_canvas->width / 3, SQUARE_SIZE*9});

#if DEBUG
        draw_grid(g_canvas);
#endif

        gx_canvas_draw(g_canvas);
        msleep(SPEED);
    }
}
