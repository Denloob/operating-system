#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>

void switch_to_text_mode();
void screen_cleanup();

void die(const char *message)
{
    switch_to_text_mode();
    screen_cleanup();

    printf("scr: %s\n", message);
    exit(1);
}

/********************/
/* Graphics library */
/********************/

enum {
    vga_char_device_CONFIG_TEXT     = '0',
    vga_char_device_CONFIG_GRAPHICS = '1',
};

void switch_to_graphics_mode()
{
    FILE *fp = fopen("/dev/vga/config", "w");
    if (fp == NULL) die("fopen: can't open /dev/vga/config");

    uint8_t current_mode = vga_char_device_CONFIG_GRAPHICS;
    size_t items_written = fwrite(&current_mode, sizeof(current_mode), 1, fp);
    if (items_written != 1) die("fwrite: can't write to /dev/vga/config");

    fclose(fp);
}

void switch_to_text_mode()
{
    // Here we can't write to stdout (we are in graphics mode) on error, so it's better we just crash if something goes wrong
    FILE *fp = fopen("/dev/vga/config", "w");

    uint8_t current_mode = vga_char_device_CONFIG_TEXT;
    size_t items_written = fwrite(&current_mode, sizeof(current_mode), 1, fp);
    if (items_written != 1) asm ("ud2" ::: "memory");

    fclose(fp);
}

#define VGA_GRAPHICS_WIDTH 320
#define VGA_GRAPHICS_HEIGHT 200
uint8_t g_screen_buf[VGA_GRAPHICS_HEIGHT * VGA_GRAPHICS_WIDTH];

static FILE *g_internal_screen_fp = NULL;
void screen_flush()
{
    if (g_internal_screen_fp == NULL)
    {
        g_internal_screen_fp = fopen("/dev/vga/screen", "w");
        if (g_internal_screen_fp == NULL) die("fopen: can't open '/dev/vga/screen'");
    }
    else
    {
        rewind(g_internal_screen_fp);
    }

    size_t items_written = fwrite(g_screen_buf, sizeof(g_screen_buf), 1, g_internal_screen_fp);
    if (items_written != 1) die("fwrite: can't write to '/dev/vga/screen'");
}

void screen_cleanup()
{
    if (g_internal_screen_fp == NULL)
    {
        return;
    }

    fclose(g_internal_screen_fp);
}

/*********************/
/* Screensaver logic */
/*********************/

#define COLOR_WHITE 7
#define COLOR_BLACK 0
#define BALL_SIZE 9
#define SPEED 40

typedef struct {
    int x;
    int y;
} Vec2;

typedef struct {
    Vec2 pos;
    Vec2 velocity;
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

    if (x >= VGA_GRAPHICS_WIDTH - radius - 1 || x <= radius)
    {
        ball->velocity.x = -dx;
    }

    if (y >= VGA_GRAPHICS_HEIGHT - radius - 1 || y <= radius)
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

            if (px >= 0 && px < VGA_GRAPHICS_WIDTH && py >= 0 && py < VGA_GRAPHICS_HEIGHT)
            {
                g_screen_buf[(py * VGA_GRAPHICS_WIDTH) + px] = ball->color;
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
    switch_to_graphics_mode();

    Ball *balls[10] = {0};
    const int ball_count = sizeof(balls) / sizeof(*balls);
    for (int i = 0; i < ball_count; i++)
    {
        int x = VGA_GRAPHICS_WIDTH / 2 + rand() % BALL_SIZE;
        int y = VGA_GRAPHICS_HEIGHT / 2 + rand() % BALL_SIZE;
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

        screen_flush();
        msleep(SPEED);
    }
}
