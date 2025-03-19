#include <assert.h>
#include <fcntl.h>
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

int g_score = 0;

void die(const char *message)
{
    gx_deinit();

    printf("tetris: %s \nyour score was: %d\n", message , g_score);
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

void shape_block_outline(gx_Canvas *canvas, gx_Vec2 pos, gx_Color color_base)
{
    gx_draw_rect_wh(canvas, pos, BLOCK_SIZE, BLOCK_SIZE, color_base + 1);

    const gx_Vec2 bottom_of_block = gx_vec2_add(pos, (gx_Vec2){ 0, BLOCK_SIZE - 1});
    gx_draw_line(canvas, bottom_of_block, gx_vec2_add(bottom_of_block, (gx_Vec2) {BLOCK_SIZE - 1, 0}), color_base + 2);
    gx_draw_line(canvas, gx_vec2_add(pos, (gx_Vec2){ BLOCK_SIZE - 1, 0}), gx_vec2_add(pos, (gx_Vec2){BLOCK_SIZE - 1, BLOCK_SIZE - 1}), color_base + 2);
}

// @param color_base - The base color of the block, `color_base + 1` is the highlight and `+2` is the shadow.
void shape_block(gx_Canvas *canvas, gx_Vec2 pos, gx_Color color_base)
{
    gx_draw_fill_rect_wh(canvas, pos, BLOCK_SIZE, BLOCK_SIZE, color_base);
    shape_block_outline(canvas, pos, color_base);
}

void shape_block_on_board_with(gx_Canvas *canvas, gx_Vec2 grid_pos, gx_Color color_base, typeof(shape_block) func)
{
    func(canvas, (gx_Vec2){BOARD_BEGIN + (grid_pos.x * BLOCK_SIZE), grid_pos.y * BLOCK_SIZE}, color_base);
}
void shape_block_on_board(gx_Canvas *canvas, gx_Vec2 grid_pos, gx_Color color_base)
{
    shape_block_on_board_with(canvas, grid_pos, color_base, shape_block);
}

void shape_block_outline_on_board(gx_Canvas *canvas, gx_Vec2 grid_pos, gx_Color color_base)
{
    shape_block_on_board_with(canvas, grid_pos, color_base, shape_block_outline);
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

void draw_piece_with(gx_Canvas *canvas, Piece piece, typeof(shape_block_on_board) func)
{
    PieceType type = piece.type;
    Rotation rotation = piece.rotation;
    gx_Vec2 pos = piece.pos;

    assert(type >= 0 && type < PIECE_COUNT && rotation <= ROTATION_270 && rotation >= ROTATION_0);

    const int block_count = sizeof(piece_offsets[type][rotation]) / sizeof(*piece_offsets[type][rotation]);
    for (int i = 0; i < block_count; i++) {
        gx_Vec2 offset = piece_offsets[type][rotation][i];
        gx_Vec2 block_pos = gx_vec2_add(pos, offset);
        func(canvas, block_pos, piece_colors[type]);
    }
}

void draw_piece(gx_Canvas *canvas, Piece piece)
{
    draw_piece_with(canvas, piece, shape_block_on_board);
}

void draw_piece_outline(gx_Canvas *canvas, Piece piece)
{
    draw_piece_with(canvas, piece, shape_block_outline_on_board);
}

#define NEXT_PIECE_SQUARE_OFFSET_X (BOARD_BLOCKS_LENGTH + 2)
#define NEXT_PIECE_SQUARE_OFFSET_Y (2)

void draw_next_piece(gx_Canvas *canvas, PieceType piece_type)
{
    Piece piece = {
        .type = piece_type,
        .pos = {.x = NEXT_PIECE_SQUARE_OFFSET_X + 2, .y = NEXT_PIECE_SQUARE_OFFSET_Y + 2}
    };
    draw_piece(canvas, piece);
}

typedef struct {
    // Each value is either 0 (empty) or a `COLOR_*_BASE` of a block
    uint8_t taken_blocks[BOARD_BLOCKS_HEIGHT][BOARD_BLOCKS_LENGTH];
    Piece current_piece;
    PieceType next_piece_type;
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

void piece_move_max_down(Piece *piece, const uint8_t taken_blocks[static BOARD_BLOCKS_HEIGHT][BOARD_BLOCKS_LENGTH]);

void draw_where_piece_would_fall(gx_Canvas *canvas, Piece piece, const uint8_t taken_blocks[static BOARD_BLOCKS_HEIGHT][BOARD_BLOCKS_LENGTH])
{
    Piece ghost = piece;

    piece_move_max_down(&ghost, taken_blocks);

    draw_piece_outline(canvas, ghost);
}

void draw_game(const Game *game)
{
    memset(g_canvas->buf, COLOR_BLACK, g_canvas->width * g_canvas->height);
    tile_background(g_canvas);
    draw_piece(g_canvas, game->current_piece);
    draw_where_piece_would_fall(g_canvas, game->current_piece, game->taken_blocks);
    draw_next_piece(g_canvas, game->next_piece_type);
    draw_taken_blocks(g_canvas, game->taken_blocks);

    gx_canvas_draw(g_canvas);
}

/************************
 *        Input
 ************************/

#define KEY_NONE        -1
#define KEY_ARROW_LEFT  0x1b
#define KEY_ARROW_RIGHT 0x1a
#define KEY_ARROW_UP    0x18
#define KEY_ARROW_DOWN  0x19
#define KEY_SPACE       ' '
#define KEY_UNKNOWN     0xa8

int g_tty_fd;

void input_init()
{
    g_tty_fd = open("/dev/tty", O_NONBLOCK | O_RDONLY);
    if (g_tty_fd < 0)
    {
        die("open: /dev/tty");
    }
}

int input_check_key()
{
    uint8_t ch = 0;
    int bytes_read = read(g_tty_fd, &ch, 1);
    if (bytes_read == 0)
    {
        return KEY_NONE;
    }

    return ch;
}



/************************
 *      Game Logic
 ************************/

#define DROP_CYCLE_MS 500

void move_piece(Piece *piece , int dx , int dy) //not a derative
{
    piece->pos.x += dx;
    piece->pos.y += dy;
}
//return true if there will be a collision and false if not
bool check_collision(Piece piece, int dx, int dy, const uint8_t taken_blocks[static BOARD_BLOCKS_HEIGHT][BOARD_BLOCKS_LENGTH])
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

// Moves the given piece as much down as possible
void piece_move_max_down(Piece *piece, const uint8_t taken_blocks[static BOARD_BLOCKS_HEIGHT][BOARD_BLOCKS_LENGTH])
{
    while (!check_collision(*piece, 0, 1, taken_blocks))
    {
        move_piece(piece, 0, 1);
    }
}

void init_game(Game *game)
{
    *game = (Game){
        .current_piece = {
            .type = rand() % PIECE_COUNT,
            .rotation = ROTATION_0,
            .pos = {3, 0},
        },
        .next_piece_type = rand() % PIECE_COUNT,
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
        .type = game->next_piece_type,
        .rotation = ROTATION_0,
        .pos = {3, 0},
    };

    if (check_collision(new_piece, 0, 0, game->taken_blocks))
    {
        die("Game Over!");
    }

    game->current_piece = new_piece;
}

void update_next_piece(Game *game)
{
    game->next_piece_type = rand() % PIECE_COUNT;
}

void handle_input(Game *game)
{
    int key = input_check_key();

    switch (key)
    {
        default:
            break;

        case KEY_ARROW_LEFT:
            if (!check_collision(game->current_piece, -1, 0, game->taken_blocks))
                move_piece(&game->current_piece, -1, 0);
            break;

        case KEY_ARROW_RIGHT:
            if (!check_collision(game->current_piece, 1, 0, game->taken_blocks))
                move_piece(&game->current_piece, 1, 0);
            break;

        case KEY_ARROW_UP:
        {
            int old_rotation = game->current_piece.rotation;

            game->current_piece.rotation = (game->current_piece.rotation + 1) % (ROTATION_270 + 1);

            if (check_collision(game->current_piece, 0, 0, game->taken_blocks))
            {
                game->current_piece.rotation = old_rotation;
            }
            break;
        }

        case KEY_ARROW_DOWN:
            if (!check_collision(game->current_piece, 0, 1, game->taken_blocks))
                move_piece(&game->current_piece, 0, 1);
            break;

        case KEY_SPACE:
            piece_move_max_down(&game->current_piece, game->taken_blocks);
            break;
    }
}

void clear_full_lines(Game *game)
{
    int lines_cleared =0;
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
            lines_cleared++;
        }
    }
    switch (lines_cleared)
    {
            case 1: g_score += 40; break;
            case 2: g_score += 100; break;
            case 3: g_score += 300; break;
            case 4: g_score += 500; break;
            case 5: g_score += 600; break;
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
        update_next_piece(game);
    }
}


int main(int argc, char **argv)
{
    srand(pit_time());
    int ret = gx_init();
    if (ret < 0) die("gx_init: couldn't initialize gx");

    g_canvas = gx_canvas_create();
    if (g_canvas == NULL) die("gx_canvas_create: out of memory");

    gx_palette_set(0, g_color_palette, sizeof(g_color_palette) / sizeof(*g_color_palette));

    input_init();

    Game game;
    init_game(&game);
    uint64_t last_drop_time = pit_time();

    while (1)
    {
        handle_input(&game);

        uint64_t now = pit_time();
        if (now - last_drop_time >= DROP_CYCLE_MS)
        {
            update_game(&game);//Drop cycle
            last_drop_time = now;
        }

        draw_game(&game);//redraw the board after the turn was played
    }
}
