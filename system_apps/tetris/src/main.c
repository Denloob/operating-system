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

/************************
 *       Graphics
 ************************/

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

typedef struct {
    // Each value is either 0 (empty) or a `COLOR_*_BASE` of a block
    uint8_t taken_blocks[BOARD_BLOCKS_HEIGHT][BOARD_BLOCKS_LENGTH];
    Piece current_piece;
} Game;

void draw_taken_blocks(gx_Canvas *canvas, const uint8_t taken_blocks[static BOARD_BLOCKS_HEIGHT][BOARD_BLOCKS_LENGTH])
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

void draw_game(const Game *game)
{
    memset(g_canvas->buf, COLOR_BLACK, g_canvas->width * g_canvas->height);
    tile_background(g_canvas);
    draw_piece(g_canvas, game->current_piece);
    draw_taken_blocks(g_canvas, game->taken_blocks);
    gx_canvas_draw(g_canvas);
}

/************************
 *      Game Logic
 ************************/

void move_piece(Piece *piece , int dx , int dy) //not a derative
{
    piece->pos.x += dx;
    piece->pos.y += dy;
}
//return true if there will be a collision and false if not
bool check_collision(Piece piece, int dx, int dy, uint8_t taken_blocks[static BOARD_BLOCKS_HEIGHT][BOARD_BLOCKS_LENGTH])
{
    PieceType type = piece.type;
    Rotation rotation = piece.rotation;
    gx_Vec2 pos = piece.pos;

    pos.x += dx;
    pos.y += dy;

    assert(type >= 0 && type < PIECE_COUNT && rotation >= ROTATION_0 && rotation <= ROTATION_270);

    const int block_count = sizeof(piece_offsets[type][rotation]) / sizeof(*piece_offsets[type][rotation]);
    for (int i = 0; i < block_count; i++)
    {
        gx_Vec2 offset = piece_offsets[type][rotation][i];
        gx_Vec2 block_pos = gx_vec2_add(pos, offset);

        if (block_pos.x < 0 || block_pos.x >= BOARD_BLOCKS_LENGTH ||
            block_pos.y < 0 || block_pos.y >= BOARD_BLOCKS_HEIGHT) {
            return true;  // Collision with wall or floor/ceiling
        }

        if (taken_blocks[block_pos.y][block_pos.x] != COLOR_BLACK)
        {
            return true;
        }
    }

    return false;
}

void init_game(Game *game)
{
    *game = (Game){
        .current_piece = {
            .type = rand() % PIECE_COUNT,
            .rotation = ROTATION_0,
            .pos = {3, 0},
        },
        .taken_blocks = {
            [BOARD_BLOCKS_HEIGHT - 1][0] = COLOR_I_BASE,
            [BOARD_BLOCKS_HEIGHT - 1][1] = COLOR_I_BASE,
            [BOARD_BLOCKS_HEIGHT - 1][2] = COLOR_I_BASE,
            [BOARD_BLOCKS_HEIGHT - 1][3] = COLOR_I_BASE,
            //[BOARD_BLOCKS_HEIGHT - 1][4] = COLOR_O_BASE,
            [BOARD_BLOCKS_HEIGHT - 1][5] = COLOR_O_BASE,
            [BOARD_BLOCKS_HEIGHT - 1][6] = COLOR_O_BASE,
            [BOARD_BLOCKS_HEIGHT - 1][7] = COLOR_O_BASE,
            [BOARD_BLOCKS_HEIGHT - 1][8] = COLOR_O_BASE,
            [BOARD_BLOCKS_HEIGHT - 1][9] = COLOR_O_BASE,
            //[BOARD_BLOCKS_HEIGHT - 2][4] = COLOR_O_BASE,
            //[BOARD_BLOCKS_HEIGHT - 2][5] = COLOR_O_BASE,
        }
    };
}

void lock_piece_into_board(Game *game)
{
    PieceType type = game->current_piece.type;
    Rotation rotation = game->current_piece.rotation;
    gx_Vec2 pos = game->current_piece.pos;

    for (int i = 0; i < 4; i++)
    {
        gx_Vec2 offset = piece_offsets[type][rotation][i];
        gx_Vec2 block_pos = gx_vec2_add(pos, offset);
        if (block_pos.y >= 0 && block_pos.y < BOARD_BLOCKS_HEIGHT &&
            block_pos.x >= 0 && block_pos.x < BOARD_BLOCKS_LENGTH)
        {
            game->taken_blocks[block_pos.y][block_pos.x] = piece_colors[type];
        }
    }
}

void spawn_new_piece(Game *game)
{
    Piece new_piece = {
        .type = rand() % PIECE_COUNT,
        .rotation = ROTATION_0,
        .pos = {3, 0},
    };

    if (check_collision(new_piece, 0, 0, game->taken_blocks))
    {
        die("Game Over!");
    }

    game->current_piece = new_piece;
}

void clear_full_lines(Game *game)
{
    for (int y = BOARD_BLOCKS_HEIGHT - 1; y >= 0; y--)
    {
        bool is_full = true;
        for (int x = 0; x < BOARD_BLOCKS_LENGTH; x++)
        {
            if (game->taken_blocks[y][x] == 0)
            {
                is_full = false;
                break;
            }
        }

        if (is_full)
        {
            for (int row = y; row > 0; row--)
            {
                for (int col = 0; col < BOARD_BLOCKS_LENGTH; col++)
                {
                    game->taken_blocks[row][col] = game->taken_blocks[row - 1][col];
                }
            }

            //delete the good line from the board
            for (int col = 0; col < BOARD_BLOCKS_LENGTH; col++)
            {
                game->taken_blocks[0][col] = 0;
            }

            //Because we cleared a raw it may be full again so we go up by one to recheck the same raw
            y++;
        }
    }
}

void update_game(Game *game)
{
    if (!check_collision(game->current_piece, 0, 1, game->taken_blocks))
    {
        move_piece(&game->current_piece, 0, 1);
    }
    else
    {
        lock_piece_into_board(game);
        clear_full_lines(game);
        spawn_new_piece(game);
    }
}

int main(int argc, char **argv)
{
    int ret = gx_init();
    if (ret < 0) die("gx_init: couldn't initialize gx");

    g_canvas = gx_canvas_create();
    if (g_canvas == NULL) die("gx_canvas_create: out of memory");

    gx_palette_set(0, g_color_palette, sizeof(g_color_palette) / sizeof(*g_color_palette));

    Game game;
    init_game(&game);
    while (1)
    {
        draw_game(&game);
        msleep(SPEED);
        update_game(&game);
    }
}
