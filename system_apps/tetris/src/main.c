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
#include "colors.h"
#include "gx/palette.h"

void die(const char *message)
{
    gx_deinit();

    printf("tetris: %s\n", message);
    exit(1);
}

gx_Canvas *g_canvas;

#define SPEED 1000

#define SQUARE_SIZE 10

int game_board_begin;
int game_board_end;

// @param color_base - The base color of the square, `color_base + 1` is the highlight and `+2` is the shadow.
void shape_square(gx_Canvas *canvas, gx_Vec2 pos, gx_Color color_base)
{
    gx_draw_fill_rect_wh(canvas, pos, SQUARE_SIZE, SQUARE_SIZE, color_base);
    gx_draw_rect_wh(canvas, pos, SQUARE_SIZE, SQUARE_SIZE, color_base + 1);

    const gx_Vec2 bottom_of_square = gx_vec2_add(pos, (gx_Vec2){ 0, SQUARE_SIZE - 1});
    gx_draw_line(canvas, bottom_of_square, gx_vec2_add(bottom_of_square, (gx_Vec2) {SQUARE_SIZE - 1, 0}), color_base + 2);
    gx_draw_line(canvas, gx_vec2_add(pos, (gx_Vec2){ SQUARE_SIZE - 1, 0}), gx_vec2_add(pos, (gx_Vec2){SQUARE_SIZE - 1, SQUARE_SIZE - 1}), color_base + 2);
}

void shape_L(gx_Canvas *canvas, gx_Vec2 pos)
{
    shape_square(canvas, gx_vec2_add(pos, (gx_Vec2){ 0, 0 }), COLOR_L_BASE);
    shape_square(canvas, gx_vec2_add(pos, (gx_Vec2){ 0, SQUARE_SIZE }), COLOR_L_BASE);
    shape_square(canvas, gx_vec2_add(pos, (gx_Vec2){ 0, 2 * SQUARE_SIZE }), COLOR_L_BASE);
    shape_square(canvas, gx_vec2_add(pos, (gx_Vec2){ 0, 3 * SQUARE_SIZE }), COLOR_L_BASE);
    shape_square(canvas, gx_vec2_add(pos, (gx_Vec2){ SQUARE_SIZE, 3 * SQUARE_SIZE }), COLOR_L_BASE);
}

void tile_background_at(gx_Canvas *canvas, gx_Vec2 start_pos, int horizontal_count)
{
#define BACKGROUND_BLOCKS_PER_SIDE 11
#define BOARD_BLOCKS_LENGTH 10
#define BOARD_BLOCKS_HEIGHT 19
    for (int x = start_pos.x; x < start_pos.x + horizontal_count * SQUARE_SIZE; x += SQUARE_SIZE)
    {
        for (int y = start_pos.y; y < canvas->height; y += SQUARE_SIZE)
        {
            shape_square(canvas, (gx_Vec2){x, y}, COLOR_BG_BLOCK_BASE);
        }
    }
}

int main(int argc, char **argv)
{
    int ret = gx_init();
    if (ret < 0) die("gx_init: couldn't initialize gx");

    g_canvas = gx_canvas_create();
    if (g_canvas == NULL) die("gx_canvas_create: out of memory");

    gx_palette_set(0, g_color_palette, sizeof(g_color_palette) / sizeof(*g_color_palette));

    game_board_begin = g_canvas->width / 3;
    game_board_end = g_canvas->width * 2 / 3;

    while (1)
    {
        memset(g_canvas->buf, COLOR_BLACK, g_canvas->width * g_canvas->height);

        const int screen_begin = (SQUARE_SIZE * (BACKGROUND_BLOCKS_PER_SIDE));

        tile_background_at(g_canvas, (gx_Vec2){screen_begin - SQUARE_SIZE, 0}, 1);
        tile_background_at(g_canvas, (gx_Vec2){screen_begin + SQUARE_SIZE * BACKGROUND_BLOCKS_PER_SIDE, 0}, 1);
        tile_background_at(g_canvas, (gx_Vec2){screen_begin, BOARD_BLOCKS_HEIGHT * SQUARE_SIZE}, BOARD_BLOCKS_LENGTH + 1);

        shape_L(g_canvas, (gx_Vec2){screen_begin + SQUARE_SIZE, SQUARE_SIZE});

        shape_L(g_canvas, (gx_Vec2){screen_begin, SQUARE_SIZE*3});
        shape_L(g_canvas, (gx_Vec2){screen_begin, SQUARE_SIZE*9});

        gx_canvas_draw(g_canvas);
        msleep(SPEED);
    }
}
