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
gx_palette_Color g_color_palette[256] = {
    [COLOR_WHITE] = {0xff, 0xff, 0xff}, // White
    [COLOR_BLACK] = {0x00, 0x00, 0x00}, // Black
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

typedef struct {
    int player_pos;
    double player_velocity;
    int enemy_pos;

    gx_Vec2f ball_pos;
    gx_Vec2f ball_velocity;
} Game;

void count_match_as_lost()
{
    int delta = pit_time() - g_game_begin_time;
    if (delta > g_longest_time) g_longest_time = delta;
}

void count_match_as_won()
{
    gx_deinit();

    puts("You won!!!");

    exit(0);
}

// Returns randomly either -1 or 1
int rand_sign()
{
    return ((rand() % 2) == 0) ? -1 : 1;
}

#define BALL_RADIUS 3
#define PLANK_HEIGHT 30
#define PLANK_WIDTH 2
#define PLAYER_X_DELTA 20
#define ENEMY_X_DELTA 20

void reset_match(Game *game)
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
    };

    g_game_begin_time = pit_time();
}

bool is_colliding_with_plank(int plank_y, gx_Vec2f ball_pos, bool is_enemy)
{
    return (
        ball_pos.y > (plank_y - BALL_RADIUS) && ball_pos.y < (plank_y + PLANK_HEIGHT + BALL_RADIUS)
        && ((!is_enemy && (ball_pos.x - BALL_RADIUS) <= PLAYER_X_DELTA + PLANK_WIDTH) || (is_enemy && (ball_pos.x + BALL_RADIUS * 2) >= (g_canvas->width - (ENEMY_X_DELTA + PLANK_WIDTH))))
    );
}

void auto_move_enemy(Game *game)
{
    if (game->ball_pos.x == 0 || game->ball_velocity.x <= 0) return;

    int final_y = (int)(game->ball_pos.y + (((g_canvas->width - game->ball_pos.x) / game->ball_velocity.x) * game->ball_velocity.y));
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
                game->ball_velocity.y = game->player_velocity * 1.5;

                const int cur_time = pit_time();
                while (pit_time() - cur_time < 700)
                {
                    memset(g_canvas->buf, COLOR_BLACK, g_canvas->height * g_canvas->width);
#define RANDOM_SHAKE (rand() % 5 - 2)

                    gx_draw_fill_circle(g_canvas, (gx_Vec2){game->ball_pos.x + RANDOM_SHAKE, game->ball_pos.y + RANDOM_SHAKE}, BALL_RADIUS, COLOR_WHITE);
                    gx_draw_rect_wh(g_canvas, (gx_Vec2){PLAYER_X_DELTA + RANDOM_SHAKE, game->player_pos + RANDOM_SHAKE}, PLANK_WIDTH, PLANK_HEIGHT, COLOR_WHITE);
                    gx_draw_rect_wh(g_canvas, (gx_Vec2){g_canvas->width - ENEMY_X_DELTA, game->enemy_pos}, PLANK_WIDTH, PLANK_HEIGHT, COLOR_WHITE);

                    gx_draw_rect_wh(g_canvas, (gx_Vec2){RANDOM_SHAKE, RANDOM_SHAKE}, g_canvas->width, g_canvas->height, COLOR_WHITE);

                    gx_canvas_draw(g_canvas);
                }
            }

        }
    }
}

void game_tick(Game *game)
{
    const double x = game->ball_pos.x;
    bool player_lost = x < 0;
    if (player_lost)
    {
        count_match_as_lost();
        reset_match(game);
        return;
    }

    bool player_won = x >= g_canvas->width;
    if (player_won)
    {
        count_match_as_won();
        reset_match(game);
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

void game_draw(Game *game)
{
    memset(g_canvas->buf, COLOR_BLACK, g_canvas->height * g_canvas->width);

    gx_draw_fill_circle(g_canvas, (gx_Vec2){game->ball_pos.x, game->ball_pos.y}, BALL_RADIUS, COLOR_WHITE);
    gx_draw_rect_wh(g_canvas, (gx_Vec2){PLAYER_X_DELTA, game->player_pos}, PLANK_WIDTH, PLANK_HEIGHT, COLOR_WHITE);
    gx_draw_rect_wh(g_canvas, (gx_Vec2){g_canvas->width - ENEMY_X_DELTA, game->enemy_pos}, PLANK_WIDTH, PLANK_HEIGHT, COLOR_WHITE);

    gx_draw_rect_wh(g_canvas, (gx_Vec2){0, 0}, g_canvas->width, g_canvas->height, COLOR_WHITE);

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
    reset_match(&game);

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
