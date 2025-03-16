#include <stdint.h>
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

    printf("scr: %s\n", message);
    exit(1);
}

gx_Canvas *g_canvas;

#define COLOR_WHITE 7
#define COLOR_BLACK 0
#define BALL_SIZE 9
#define SPEED 40

typedef struct {
    gx_Vec2 pos;
    gx_Vec2 velocity;
    int radius;
    int color;
} Ball;

void ball_tick(Ball *ball)
{
    const int dx = ball->velocity.x;
    const int dy = ball->velocity.y;
    const int radius = ball->radius;
    int x = ball->pos.x += dx;
    int y = ball->pos.y += dy;

    if (x >= g_canvas->width - radius - 1 || x <= radius)
    {
        ball->velocity.x = -dx;
    }

    if (y >= g_canvas->height - radius - 1 || y <= radius)
    {
        ball->velocity.y = -dy;
    }
}

void ball_draw(Ball *ball)
{
    const int radius = ball->radius;
    const int x = ball->pos.x;
    const int y = ball->pos.y;
    for (int i = -radius; i < radius; i++)
    {
        for (int j = -radius; j < radius; j++)
        {
            const int px = x + i;
            const int py = y + j;

            if (px >= 0 && px < g_canvas->width && py >= 0 && py < g_canvas->height)
            {
                g_canvas->buf[(py * g_canvas->width) + px] = ball->color;
            }
        }
    }
}

Ball *ball_create(int x, int y, int radius, int dx, int dy, int color)
{
    Ball *ball = malloc(sizeof(*ball));
    if (ball == NULL) die("malloc: out of memory");

    ball->pos.x = x;
    ball->pos.y = y;

    ball->velocity.x = dx;
    ball->velocity.y = dy;

    ball->radius = radius;
    ball->color  = color;

    return ball;
}

int main(int argc, char **argv)
{
    int ret = gx_init();
    if (ret < 0) die("gx_init: couldn't initialize gx");

    g_canvas = gx_canvas_create();
    if (g_canvas == NULL) die("gx_canvas_create: out of memory");

    Ball *balls[10] = {0};
    const int ball_count = sizeof(balls) / sizeof(*balls);
    for (int i = 0; i < ball_count; i++)
    {
        int x = g_canvas->width / 2 + rand() % BALL_SIZE;
        int y = g_canvas->height / 2 + rand() % BALL_SIZE;
        int dx = (rand() % BALL_SIZE) - BALL_SIZE / 2;
        int dy = (rand() % BALL_SIZE) - BALL_SIZE / 2;
        balls[i] = ball_create(x, y, BALL_SIZE, dx, dy, i + 1);
    }

    while (true)
    {
        for (int i = 0; i < ball_count; i++)
        {
            ball_tick(balls[i]);
            ball_draw(balls[i]);
        }

        gx_canvas_draw(g_canvas);
        msleep(SPEED);
    }
}
