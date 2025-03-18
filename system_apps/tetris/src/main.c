#include <assert.h>
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

#define BACKGROUND_BLOCKS_PER_SIDE 11
#define BOARD_BLOCKS_LENGTH 10
#define BOARD_BLOCKS_HEIGHT 19

#define SPEED 1000

#define BLOCK_SIZE 10

#define BOARD_BEGIN (BLOCK_SIZE * (BACKGROUND_BLOCKS_PER_SIDE))

// @param color_base - The base color of the block, `color_base + 1` is the highlight and `+2` is the shadow.
void shape_block(gx_Canvas *canvas, gx_Vec2 pos, gx_Color color_base)
{
    gx_draw_fill_rect_wh(canvas, pos, BLOCK_SIZE, BLOCK_SIZE, color_base);
    gx_draw_rect_wh(canvas, pos, BLOCK_SIZE, BLOCK_SIZE, color_base + 1);

    const gx_Vec2 bottom_of_block = gx_vec2_add(pos, (gx_Vec2){ 0, BLOCK_SIZE - 1});
    gx_draw_line(canvas, bottom_of_block, gx_vec2_add(bottom_of_block, (gx_Vec2) {BLOCK_SIZE - 1, 0}), color_base + 2);
    gx_draw_line(canvas, gx_vec2_add(pos, (gx_Vec2){ BLOCK_SIZE - 1, 0}), gx_vec2_add(pos, (gx_Vec2){BLOCK_SIZE - 1, BLOCK_SIZE - 1}), color_base + 2);
}

void shape_block_on_board(gx_Canvas *canvas, gx_Vec2 grid_pos, gx_Color color_base)
{
    shape_block(canvas, (gx_Vec2){BOARD_BEGIN + (grid_pos.x * BLOCK_SIZE), grid_pos.y * BLOCK_SIZE}, color_base);
}

void tile_background_at(gx_Canvas *canvas, gx_Vec2 start_pos, int horizontal_count)
{
    for (int x = start_pos.x; x < start_pos.x + horizontal_count; x++)
    {
        for (int y = start_pos.y; y < BOARD_BLOCKS_HEIGHT + 1; y++)
        {
            shape_block_on_board(canvas, (gx_Vec2){x, y}, COLOR_BG_BLOCK_BASE);
        }
    }
}

void tile_background(gx_Canvas *canvas)
{
    // We draw outside the bounds of the game board, which is then normalized to be on the screen around it
    tile_background_at(g_canvas, (gx_Vec2){ -1, 0 }, 1);
    tile_background_at(g_canvas, (gx_Vec2){ BOARD_BLOCKS_LENGTH, 0 }, 1);
    tile_background_at(g_canvas, (gx_Vec2){ 0, BOARD_BLOCKS_HEIGHT}, BOARD_BLOCKS_LENGTH);
}

typedef enum {
    PIECE_I,
    PIECE_J,
    PIECE_L,
    PIECE_O,
    PIECE_S,
    PIECE_T,
    PIECE_Z,
    PIECE_COUNT,
} PieceType;

typedef enum {
    ROTATION_0,
    ROTATION_90,
    ROTATION_180,
    ROTATION_270,
} Rotation;

typedef struct {
    gx_Vec2 pos;
    PieceType type;
    Rotation rotation;
} Piece;

static const gx_Vec2 piece_offsets[PIECE_COUNT][4][4] = {
    [PIECE_I] = {
        [ROTATION_0]   = {{0,0}, {1,0}, {2,0}, {3,0}},
        [ROTATION_90]  = {{0,0}, {0,1}, {0,2}, {0,3}},
        [ROTATION_180] = {{0,0}, {1,0}, {2,0}, {3,0}},
        [ROTATION_270] = {{0,0}, {0,1}, {0,2}, {0,3}},
    },
    [PIECE_J] = {
        [ROTATION_0]   = {{0,0}, {0,1}, {0,2}, {1,2}},
        [ROTATION_90]  = {{0,0}, {1,0}, {2,0}, {0,1}},
        [ROTATION_180] = {{0,0}, {1,0}, {1,1}, {1,2}},
        [ROTATION_270] = {{2,0}, {0,1}, {1,1}, {2,1}},
    },
    [PIECE_L] = {
        [ROTATION_0]   = {{0,0}, {0,1}, {0,2}, {1,0}},
        [ROTATION_90]  = {{0,0}, {1,0}, {2,0}, {2,1}},
        [ROTATION_180] = {{1,0}, {1,1}, {1,2}, {0,2}},
        [ROTATION_270] = {{0,1}, {1,1}, {2,1}, {0,0}},
    },
    [PIECE_O] = {
        [ROTATION_0]   = {{0,0}, {1,0}, {0,1}, {1,1}},
        [ROTATION_90]  = {{0,0}, {1,0}, {0,1}, {1,1}},
        [ROTATION_180] = {{0,0}, {1,0}, {0,1}, {1,1}},
        [ROTATION_270] = {{0,0}, {1,0}, {0,1}, {1,1}},
    },
    [PIECE_S] = {
        [ROTATION_0]   = {{0,0}, {1,0}, {1,1}, {2,1}},
        [ROTATION_90]  = {{1,0}, {1,1}, {0,1}, {0,2}},
        [ROTATION_180] = {{0,0}, {1,0}, {1,1}, {2,1}},
        [ROTATION_270] = {{1,0}, {1,1}, {0,1}, {0,2}},
    },
    [PIECE_T] = {
        [ROTATION_0]   = {{0,0}, {1,0}, {2,0}, {1,1}},
        [ROTATION_90]  = {{0,1}, {1,0}, {1,1}, {1,2}},
        [ROTATION_180] = {{1,0}, {0,1}, {1,1}, {2,1}},
        [ROTATION_270] = {{0,0}, {0,1}, {0,2}, {1,1}},
    },
    [PIECE_Z] = {
        [ROTATION_0]   = {{0,1}, {1,1}, {1,0}, {2,0}},
        [ROTATION_90]  = {{0,0}, {0,1}, {1,1}, {1,2}},
        [ROTATION_180] = {{0,1}, {1,1}, {1,0}, {2,0}},
        [ROTATION_270] = {{0,0}, {0,1}, {1,1}, {1,2}},
    }
};

_Static_assert((sizeof(piece_offsets) / sizeof(*piece_offsets)) == PIECE_COUNT, "Piece count should reflect the actual piece count");

static const uint32_t piece_colors[] = {
    [PIECE_I] = COLOR_I_BASE,
    [PIECE_J] = COLOR_J_BASE,
    [PIECE_L] = COLOR_L_BASE,
    [PIECE_O] = COLOR_O_BASE,
    [PIECE_S] = COLOR_S_BASE,
    [PIECE_T] = COLOR_T_BASE,
    [PIECE_Z] = COLOR_Z_BASE
};


typedef struct {
    // Each value is either 0 (empty) or a `COLOR_*_BASE` of a block
    uint8_t taken_blocks[BOARD_BLOCKS_HEIGHT][BOARD_BLOCKS_LENGTH];
    Piece current_piece;
} Game;

void draw_piece(gx_Canvas *canvas, Piece piece) {
    PieceType type = piece.type;
    Rotation rotation = piece.rotation;
    gx_Vec2 pos = piece.pos;

    assert(type >= 0 && type < PIECE_COUNT && rotation <= ROTATION_270 && rotation >= ROTATION_0);

    const int block_count = sizeof(piece_offsets[type][rotation]) / sizeof(*piece_offsets[type][rotation]);
    for (int i = 0; i < block_count; i++) {
        gx_Vec2 offset = piece_offsets[type][rotation][i];
        gx_Vec2 block_pos = gx_vec2_add(pos, offset);
        shape_block_on_board(canvas, block_pos, piece_colors[type]);
    }
}

void draw_taken_blocks(gx_Canvas *canvas, uint8_t taken_blocks[static BOARD_BLOCKS_HEIGHT][BOARD_BLOCKS_LENGTH])
{
    for (int x = 0; x < BOARD_BLOCKS_LENGTH; x++)
    {
        for (int y = 0; y < BOARD_BLOCKS_HEIGHT; y++)
        {
            int color = taken_blocks[y][x];
            if (color == 0)
            {
                continue;
            }

            shape_block_on_board(canvas, (gx_Vec2){x, y}, color);
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

    Game game = {
        .current_piece = {
            .type = PIECE_Z,
        },
        .taken_blocks = {
            [BOARD_BLOCKS_HEIGHT - 1][0] = COLOR_I_BASE,
            [BOARD_BLOCKS_HEIGHT - 1][1] = COLOR_I_BASE,
            [BOARD_BLOCKS_HEIGHT - 1][2] = COLOR_I_BASE,
            [BOARD_BLOCKS_HEIGHT - 1][3] = COLOR_I_BASE,
            [BOARD_BLOCKS_HEIGHT - 1][4] = COLOR_O_BASE,
            [BOARD_BLOCKS_HEIGHT - 1][5] = COLOR_O_BASE,
            [BOARD_BLOCKS_HEIGHT - 2][4] = COLOR_O_BASE,
            [BOARD_BLOCKS_HEIGHT - 2][5] = COLOR_O_BASE,
        }
    };
    while (1)
    {
        memset(g_canvas->buf, COLOR_BLACK, g_canvas->width * g_canvas->height);
        tile_background(g_canvas);

        draw_piece(g_canvas, game.current_piece);
        draw_taken_blocks(g_canvas, game.taken_blocks);

        gx_canvas_draw(g_canvas);
        msleep(SPEED);
    }
}
