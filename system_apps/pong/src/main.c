#include <gx/canvas.h>
#include <gx/mouse.h>
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
#include <gx/palette.h>

#define COLOR_BLACK       0
#define COLOR_WHITE       1
#define COLOR_GRAY        2
#define COLOR_RED         3
gx_palette_Color g_color_palette[256] = {
    [COLOR_WHITE] = {0xff, 0xff, 0xff}, // White
    [COLOR_BLACK] = {0x00, 0x00, 0x00}, // Black
    [COLOR_GRAY] = {0x33, 0x33, 0x33},
    [COLOR_RED] = {0xff, 0x00, 0x00},
};

#define HEART_SIZE 8
#define HEART_SCALE 2
const uint8_t g_heart_bitmap[HEART_SIZE * HEART_SIZE] = {
    0, 2, 2, 0, 0, 2, 2, 0,
    2, 3, 3, 2, 2, 3, 3, 2,
    2, 3, 3, 3, 3, 3, 3, 2,
    2, 3, 3, 3, 3, 3, 3, 2,
    0, 2, 3, 3, 3, 3, 2, 0,
    0, 0, 2, 3, 3, 2, 0, 0,
    0, 0, 0, 2, 2, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
};

const uint8_t g_heart_mask[HEART_SIZE * HEART_SIZE] = {
    0, 1, 1, 0, 0, 1, 1, 0,
    1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,
    0, 1, 1, 1, 1, 1, 1, 0,
    0, 0, 1, 1, 1, 1, 0, 0,
    0, 0, 0, 1, 1, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
};

#define HEART_BREAK_FRAMES 4
const uint8_t g_heart_break_bitmap[HEART_BREAK_FRAMES][HEART_SIZE * HEART_SIZE] = {
    {
        2, 0, 0, 2, 2, 0, 0, 2,
        0, 2, 2, 0, 0, 2, 2, 0,
        0, 2, 3, 2, 2, 3, 2, 0,
        2, 3, 3, 3, 3, 3, 3, 2,
        0, 2, 3, 3, 3, 3, 2, 0,
        0, 0, 2, 3, 3, 2, 0, 0,
        0, 0, 0, 2, 2, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
    },
    {
        2, 0, 0, 0, 0, 0, 0, 2,
        0, 2, 0, 2, 2, 0, 2, 0,
        0, 0, 2, 0, 0, 2, 0, 0,
        2, 2, 3, 3, 3, 3, 2, 2,
        0, 0, 2, 3, 3, 2, 0, 0,
        0, 0, 0, 2, 2, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
    },
    {
        2, 0, 0, 0, 0, 0, 0, 2,
        0, 2, 0, 0, 0, 0, 2, 0,
        0, 0, 0, 2, 2, 0, 0, 0,
        0, 0, 2, 0, 0, 2, 0, 0,
        0, 0, 0, 2, 2, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
    },
    {
        2, 0, 0, 0, 0, 0, 0, 2,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
    }
};

const uint8_t g_heart_break_mask[HEART_BREAK_FRAMES][HEART_SIZE * HEART_SIZE] = {
    {
        1, 0, 0, 1, 1, 0, 0, 1,
        0, 1, 1, 0, 0, 1, 1, 0,
        0, 1, 1, 1, 1, 1, 1, 0,
        1, 1, 1, 1, 1, 1, 1, 1,
        0, 1, 1, 1, 1, 1, 1, 0,
        0, 0, 1, 1, 1, 1, 0, 0,
        0, 0, 0, 1, 1, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
    },
    {
        1, 0, 0, 0, 0, 0, 0, 1,
        0, 1, 0, 1, 1, 0, 1, 0,
        0, 0, 1, 0, 0, 1, 0, 0,
        1, 1, 1, 1, 1, 1, 1, 1,
        0, 0, 1, 1, 1, 1, 0, 0,
        0, 0, 0, 1, 1, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
    },
    {
        1, 0, 0, 0, 0, 0, 0, 1,
        0, 1, 0, 0, 0, 0, 1, 0,
        0, 0, 0, 1, 1, 0, 0, 0,
        0, 0, 1, 0, 0, 1, 0, 0,
        0, 0, 0, 1, 1, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
    },
    {
        1, 0, 0, 0, 0, 0, 0, 1,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
    }
};

int g_longest_time = 0;
int g_game_begin_time;

void die(const char *message)
{
    gx_deinit();

    printf("pong: %s\n", message);
    exit(1);
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
#define KEY_Q           'q'
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

gx_Canvas *g_canvas;

#define FRAME_DELAY 20

typedef enum {
    ANIMATION_NONE,
    ANIMATION_WORLD_BREAK,
    ANIMATION_WORLD_EXPLODE,
    ANIMATION_WORLD_RESTORE,

    ANIMATION_HEART_SHAKE,
    ANIMATION_HEART_BREAK,

    ANIMATION_STRONG_HIT,
} AnimationState;

typedef struct {
    int player_pos;
    double player_velocity;
    int enemy_pos;

    gx_Vec2f ball_pos;
    gx_Vec2f ball_velocity;

    uint64_t last_animation_state_change;
    AnimationState animation_state;

    int health;
} Game;


// Returns randomly either -1 or 1
int rand_sign()
{
    return ((rand() % 2) == 0) ? -1 : 1;
}

#define STARTING_HEALTH 3
#define BALL_RADIUS 3
#define PLANK_HEIGHT 30
#define PLANK_WIDTH 2
#define PLAYER_X_DELTA 20
#define ENEMY_X_DELTA 20

void reset_match(Game *game, bool reset_health)
{
    *game = (Game){
        .ball_pos = {
            g_canvas->width / 2.0,
            g_canvas->height / 2.0,
        },
        .ball_velocity = {
            .x = rand_sign() * 7,
            .y = rand_sign() * ((rand() % 100) / 100.0),
        },
        .player_pos = gx_mouse_get_state().y,
        .enemy_pos = (g_canvas->height - PLANK_HEIGHT) / 2,
        .health = reset_health ? STARTING_HEALTH : game->health,
    };

    g_game_begin_time = pit_time();
}

void count_match_as_lost(Game *game)
{
    game->health--;
    game->last_animation_state_change = pit_time();
    game->animation_state = ANIMATION_HEART_SHAKE;
}

void count_match_as_won()
{
    gx_deinit();

    puts("You won!!!");

    exit(0);
}

bool is_colliding_with_plank(int plank_y, gx_Vec2f ball_pos, bool is_enemy)
{
    return (
        ball_pos.y > (plank_y - (is_enemy ? 0 : BALL_RADIUS * 2)) && ball_pos.y < (plank_y + PLANK_HEIGHT + (is_enemy ? 0 : BALL_RADIUS * 2))

        && ((!is_enemy &&
                ((ball_pos.x - BALL_RADIUS) <= (PLAYER_X_DELTA + PLANK_WIDTH) && (ball_pos.x - BALL_RADIUS) > (PLAYER_X_DELTA - 5)))
          || (is_enemy &&
                (ball_pos.x + BALL_RADIUS) >= (g_canvas->width - (ENEMY_X_DELTA + PLANK_WIDTH)) && (ball_pos.x + BALL_RADIUS) < (g_canvas->width - (ENEMY_X_DELTA - 5)))));
}

void auto_move_enemy(Game *game)
{
    if (game->ball_pos.x == 0 || game->ball_velocity.x <= 0) return;

    int final_y = (int)(game->ball_pos.y + (((g_canvas->width - (game->ball_pos.x + ENEMY_X_DELTA + PLANK_WIDTH)) / game->ball_velocity.x) * game->ball_velocity.y));
    final_y = abs(final_y);

    if ((final_y / g_canvas->height) % 2 == 0)
    {
        final_y %= g_canvas->height;
    }
    else
    {
        final_y = g_canvas->height - (final_y % g_canvas->height);
    }

    const int enemy_middle = game->enemy_pos + PLANK_HEIGHT / 2;
    if (abs(final_y - enemy_middle) > PLANK_HEIGHT / 4)
    {
        if (final_y > enemy_middle)
        {
            game->enemy_pos += 5;
        }
        else
        {
            game->enemy_pos -= 5;
        }
    }
}

void draw_hearts(Game *game);

void process_plank_collisions(Game *game)
{
    bool colliding_with_player = is_colliding_with_plank(game->player_pos, game->ball_pos, false);
    bool colliding_with_enemy = is_colliding_with_plank(game->enemy_pos, game->ball_pos, true);
    if (colliding_with_enemy || colliding_with_player)
    {
        game->ball_velocity.x = -game->ball_velocity.x;
        game->ball_pos.x = (colliding_with_player ? (PLAYER_X_DELTA + BALL_RADIUS) : (g_canvas->width - ENEMY_X_DELTA - BALL_RADIUS * 2)) + PLANK_WIDTH;

        if (colliding_with_player)
        {
            game->ball_velocity.y += game->player_velocity;

            if (abs(game->player_velocity) > 2) // A strong hit
            {
                game->animation_state = ANIMATION_STRONG_HIT;
                game->ball_velocity.y = game->player_velocity * 1.5;

                const int cur_time = pit_time();
                while (pit_time() - cur_time < 700)
                {
                    memset(g_canvas->buf, COLOR_BLACK, g_canvas->height * g_canvas->width);
                    draw_hearts(game);
#define RANDOM_SHAKE (rand() % 5 - 2)

                    gx_draw_fill_circle(g_canvas, (gx_Vec2){game->ball_pos.x + RANDOM_SHAKE, game->ball_pos.y + RANDOM_SHAKE}, BALL_RADIUS, COLOR_WHITE);
                    gx_draw_rect_wh(g_canvas, (gx_Vec2){PLAYER_X_DELTA + RANDOM_SHAKE, game->player_pos + RANDOM_SHAKE}, PLANK_WIDTH, PLANK_HEIGHT, COLOR_WHITE);
                    gx_draw_rect_wh(g_canvas, (gx_Vec2){g_canvas->width - ENEMY_X_DELTA, game->enemy_pos}, PLANK_WIDTH, PLANK_HEIGHT, COLOR_WHITE);

                    gx_draw_rect_wh(g_canvas, (gx_Vec2){RANDOM_SHAKE, RANDOM_SHAKE}, g_canvas->width, g_canvas->height, COLOR_WHITE);

                    gx_canvas_draw(g_canvas);
                }
            }

            game->animation_state = ANIMATION_NONE;
        }
    }
}

void game_tick(Game *game)
{
    if (game->animation_state != ANIMATION_NONE)
    {
        // Heart animation
        if (game->animation_state == ANIMATION_HEART_SHAKE && pit_time() - game->last_animation_state_change > 600)
        {
            game->animation_state = ANIMATION_HEART_BREAK;
            game->last_animation_state_change = pit_time();
        }
        else if (game->animation_state == ANIMATION_HEART_BREAK && pit_time() - game->last_animation_state_change > 400)
        {
            if (game->health == 0)
            {
                game->animation_state = ANIMATION_WORLD_BREAK;
                game->last_animation_state_change = pit_time();
            }
            else
            {
                reset_match(game, false);
            }
        }

        // World animation
        if (game->animation_state == ANIMATION_WORLD_BREAK && pit_time() - game->last_animation_state_change > 500)
        {
            game->animation_state = ANIMATION_WORLD_EXPLODE;
            game->last_animation_state_change = pit_time();
        }
        else if (game->animation_state == ANIMATION_WORLD_EXPLODE && pit_time() - game->last_animation_state_change > 2000)
        {
            reset_match(game, true); // When ANIMATION_WORLD_RESTORE is running, the match is already reset
            game->animation_state = ANIMATION_WORLD_RESTORE;
            game->last_animation_state_change = pit_time();
        }
        else if (game->animation_state == ANIMATION_WORLD_RESTORE && pit_time() - game->last_animation_state_change > 1000)
        {
            game->animation_state = ANIMATION_NONE;
            game->last_animation_state_change = pit_time();
        }
        return;
    }

    const double x = game->ball_pos.x;
    bool player_lost = x < 0;
    if (player_lost)
    {
        count_match_as_lost(game);
        return;
    }

    bool player_won = x >= g_canvas->width;
    if (player_won)
    {
        count_match_as_won();
        reset_match(game, true);
        return;
    }

    auto_move_enemy(game);

    game->ball_pos = gx_vec2f_add(game->ball_pos, game->ball_velocity);

    const double y = game->ball_pos.y;
    if (y < 0 || y >= g_canvas->height)
    {
        game->ball_velocity.y = -game->ball_velocity.y;
    }

    process_plank_collisions(game);
}

static int g_prev_mouse_y;
void input_handle(Game *game)
{
    switch (input_check_key())
    {
        case KEY_Q:
            die("terminated by user input");
            assert(false && "Unreachable");
        default:
            break; /* Do nothing */
    }


    gx_mouse_State state = gx_mouse_get_state();
    game->player_pos = state.y;

    game->player_velocity = (state.y - g_prev_mouse_y) / 5;
}

void draw_heart(gx_Canvas *canvas, gx_Vec2 pos, const uint8_t *bitmap, const uint8_t *mask)
{
    for (int x = 0; x < HEART_SIZE; x++)
    {
        for (int y = 0; y < HEART_SIZE; y++)
        {
            int heart_idx = (y * HEART_SIZE) + x;

            if (mask[heart_idx])
            {
                gx_draw_fill_rect_wh(canvas, (gx_Vec2){pos.x + x * HEART_SCALE, pos.y + y * HEART_SCALE}, HEART_SCALE, HEART_SCALE, bitmap[heart_idx]);
            }
        }
    }
}

int get_shake_cond(const Game *game, bool cond)
{
    if (cond)
        return RANDOM_SHAKE;
    return 0;
}

int get_shake(const Game *game)
{
    return get_shake_cond(game, game->animation_state == ANIMATION_WORLD_BREAK || game->animation_state == ANIMATION_STRONG_HIT);
}


void draw_heart_break_anim_frame(Game *game, gx_Vec2 pos, int frame_num)
{
    assert(frame_num < HEART_BREAK_FRAMES);

    draw_heart(g_canvas, pos, g_heart_break_bitmap[frame_num], g_heart_break_mask[frame_num]);
}

void draw_hearts(Game *game)
{
#define HEART_START_Y 10
#define HEART_START_X 30
#define HEART_PADDING 5

    int x = HEART_START_X;

    for (int i = 0; i < game->health; i++)
    {
        draw_heart(g_canvas, (gx_Vec2){x + get_shake(game), HEART_START_Y + get_shake(game)}, g_heart_bitmap, g_heart_mask);
        x += HEART_SIZE * HEART_SCALE + HEART_PADDING;
    }

    if (game->animation_state == ANIMATION_HEART_SHAKE)
    {
        draw_heart(g_canvas, (gx_Vec2){x + get_shake_cond(game, true), HEART_START_Y + get_shake_cond(game, true)}, g_heart_bitmap, g_heart_mask);
    }
    else if (game->animation_state == ANIMATION_HEART_BREAK)
    {
        int cur_animation_duration = pit_time() - game->last_animation_state_change;
        int frame_num = cur_animation_duration / 100;
        if (frame_num >= HEART_BREAK_FRAMES) frame_num = HEART_BREAK_FRAMES - 1;
        draw_heart_break_anim_frame(game, (gx_Vec2){x + get_shake(game), HEART_START_Y + get_shake(game)}, frame_num);
    }
}

void game_draw(Game *game)
{
    memset(g_canvas->buf, COLOR_BLACK, g_canvas->height * g_canvas->width);

    gx_Vec2 border_pos = {get_shake(game), get_shake(game)};
    gx_Vec2 border_size = {g_canvas->width + get_shake(game), g_canvas->height + get_shake(game)};

    if (game->animation_state == ANIMATION_WORLD_EXPLODE)
    {
        int cur_animation_duration = pit_time() - game->last_animation_state_change;
        if (cur_animation_duration < 400)
        {
            // Shrink the border on y towards center
            border_pos.y += cur_animation_duration / 4;
            border_size.y -= (cur_animation_duration / 4) * 2;
        }
        else if (cur_animation_duration < 700)
        {
            // Keep it in the center
            border_pos.y = g_canvas->height / 2;
            border_size.y = 0;
        }
        else
        {
            // Flash bang
            memset(g_canvas->buf, COLOR_WHITE, g_canvas->height * g_canvas->width);
        }
    }

    draw_hearts(game);

    gx_draw_fill_circle(g_canvas, (gx_Vec2){game->ball_pos.x + get_shake(game), game->ball_pos.y + get_shake(game)}, BALL_RADIUS, COLOR_WHITE);
    gx_draw_rect_wh(g_canvas, (gx_Vec2){PLAYER_X_DELTA + get_shake(game), game->player_pos + get_shake(game)}, PLANK_WIDTH, PLANK_HEIGHT, COLOR_WHITE);
    gx_draw_rect_wh(g_canvas, (gx_Vec2){g_canvas->width - ENEMY_X_DELTA + get_shake(game), game->enemy_pos + get_shake(game)}, PLANK_WIDTH, PLANK_HEIGHT, COLOR_WHITE);

    gx_draw_rect_wh(g_canvas, border_pos, border_size.x, border_size.y, COLOR_WHITE);

    if (game->animation_state == ANIMATION_WORLD_RESTORE)
    {
        // The game was already restored, but we hide it under the white flash, and slowly reveal it

        int cur_animation_duration = pit_time() - game->last_animation_state_change;
        int flash_height = (g_canvas->height - (cur_animation_duration / 5));
        if (flash_height < 0) flash_height = 0;
        memset(g_canvas->buf, COLOR_WHITE, g_canvas->width * flash_height);
    }

    gx_canvas_draw(g_canvas);
}

int main(int argc, char **argv)
{
    srand(pit_time());

    int ret = gx_init();
    if (ret < 0) die("gx_init: couldn't initialize gx");

    ret = gx_mouse_init();
    if (ret < 0) die("gx_mouse_init: couldn't initialize gx mouse");

    g_canvas = gx_canvas_create();
    if (g_canvas == NULL) die("gx_canvas_create: out of memory");

    gx_palette_set(0, g_color_palette, sizeof(g_color_palette) / sizeof(*g_color_palette));

    input_init();

    Game game = {};
    reset_match(&game, true);

    uint64_t prev_frame = pit_time();
    while (1)
    {
        input_handle(&game);

        if (pit_time() - prev_frame > FRAME_DELAY)
        {
            prev_frame = pit_time();
            game_tick(&game);

            g_prev_mouse_y = gx_mouse_get_state().y;
        }
        game_draw(&game);
    }
}
