#include "app.h"
#include "bmp.h"
#include "button.h"
#include <assert.h>
#include <fcntl.h>
#include <gx/canvas.h>
#include <gx/draw.h>
#include <gx/gx.h>
#include <gx/mouse.h>
#include <gx/palette.h>
#include <gx/vec.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

enum {
    COLOR__FIRST_COLOR = 254,
    COLOR_WHITE = COLOR__FIRST_COLOR,
    COLOR_BLACK,

    COLOR__LAST_COLOR,
};

#define COLOR_COUNT (COLOR__LAST_COLOR - COLOR__FIRST_COLOR)

gx_palette_Color g_color_palette[] = {
    [COLOR_WHITE - COLOR__FIRST_COLOR] = {0xff, 0xff, 0xff},
    [COLOR_BLACK - COLOR__FIRST_COLOR] = {0x00, 0x00, 0x00},
};

void die(const char *message)
{
    gx_deinit();

    printf("paint: %s\n", message);
    exit(1);
}

#define KEY_NONE        -1
#define KEY_Q           'q'

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

gx_Canvas *g_canvas;

App g_app;

#define FRAME_DELAY 20



int get_file_size(FILE *fp)
{
    int ret = fseek(fp, 0, SEEK_END);
    if (ret < 0)
        die("fseek: wallpaper");

    int file_size = ftell(fp);
    if (file_size < 0)
        die("ftell: wallpaper");

    rewind(fp);

    return file_size;
}

gx_Canvas *load_image(const char *path, int color_palette_limit)
{
    static int color_palette_offset = 0;
    assert(color_palette_offset < 256);
    FILE *image_file = fopen(path, "r");
    if (image_file == NULL)
        die("fopen: image");

    int image_bmp_size = get_file_size(image_file);

    uint8_t *image_bmp = malloc(image_bmp_size);
    if (image_bmp == NULL)
        die("malloc: out of memory");

    int bytes_read = fread(image_bmp, 1, image_bmp_size, image_file);
    if (bytes_read < image_bmp_size)
        die("fread: image");

    const bmp_DIBHeader *dib = bmp_get_dib_header(image_bmp, image_bmp_size);

    gx_Canvas *image_canvas = gx_canvas_create_of_size(dib->width, dib->height);
    if (image_canvas == NULL)
        die("gx_canvas_create_of_size");

    bmp_draw_at(image_canvas, (gx_Vec2){0, 0}, image_bmp, image_bmp_size, color_palette_offset, color_palette_limit);

    color_palette_offset += MIN(bmp_get_color_palette_length(dib), color_palette_limit);

    free(image_bmp);
    fclose(image_file);

    return image_canvas;
}

void draw_cursor_texture(gx_Canvas *canvas, gx_Vec2 pos)
{
    const uint8_t cursor[] = {
        1, 1, 0, 0, 0, 0, 0, 0,
        1, 2, 1, 0, 0, 0, 0, 0,
        1, 2, 2, 1, 0, 0, 0, 0,
        1, 2, 2, 2, 1, 0, 0, 0,
        1, 2, 2, 1, 1, 0, 0, 0,
        1, 1, 1, 2, 1, 0, 0, 0,
        0, 0, 1, 1, 1, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
    };
    const int cursor_size = 8;

    for (int x = 0; x < cursor_size; x++)
    {
        for (int y = 0; y < cursor_size; y++)
        {
            int color_number = cursor[x + y * cursor_size];
            if (color_number)
            {
                gx_draw_point(canvas, gx_vec2_add(pos, (gx_Vec2){x, y}), color_number == 1 ? COLOR_WHITE : COLOR_BLACK);
            }
        }
    }
}

void input_handle(App *app)
{
    switch (input_check_key())
    {
        case KEY_Q:
            gx_deinit();
            exit(0);
            assert(false && "Unreachable");
        default:
            break; /* Do nothing */
    }

    gx_mouse_State state = gx_mouse_get_state();

    app->mouse.pos.x = state.x;
    app->mouse.pos.y = state.y;

    bool was_pressed = app->mouse.is_pressed;
    app->mouse.is_pressed = state.buttons & gx_mouse_BUTTON_LEFT;

    app->mouse.was_just_released = was_pressed && !app->mouse.is_pressed;
}

void app_update(App *app)
{
    for (int i = 0; i < app->button_count; i++)
    {
        assert(app->buttons[i] != NULL);
        button_update(app->buttons[i], app);
    }
}

void app_display(App *app)
{
    gx_draw_canvas(g_canvas, (gx_Vec2){0, 0}, app->assets.wallpaper);

    for (int i = 0; i < app->button_count; i++)
    {
        if (app->buttons[i]->texture != NULL)
        {
            gx_draw_canvas(g_canvas, app->buttons[i]->pos, app->buttons[i]->texture);
        }
    }

    draw_cursor_texture(g_canvas, app->mouse.pos);

    gx_canvas_draw(g_canvas);
}

#define BUTTONS_SIZE 16

void button_callback(void *arg)
{
    const char *path = arg;
    execve_new(path, NULL);
}

void app_add_button(App *app, gx_Canvas *texture, const char *bin_path, gx_Vec2 pos)
{
    assert(app->button_capacity > app->button_count && "Not supported");

    Button *button = calloc(1, sizeof(*button));
    assert(button != NULL);

    button->pos = pos;

    button->callback_arg = strdup(bin_path);
    button->callback = button_callback;

    button->texture = texture;

    button->size.x = texture ? texture->width : BUTTONS_SIZE;
    button->size.y = texture ? texture->height : BUTTONS_SIZE;

    app->buttons[app->button_count] = button;
    app->button_count++;
}

#define WALLPAPER_COLOR_PALETTE_SIZE 254
#define ICONS_COLOR_PALETTE_SIZE 20

int main(int argc, char **argv)
{
    const char *wallpaper_path = "/usr/share/win-bg.bmp";
    if (argc > 1)
    {
        wallpaper_path = argv[1];
    }

    int ret = gx_init();
    if (ret < 0) die("gx_init: couldn't initialize gx");

    ret = gx_mouse_init();
    if (ret < 0) die("gx_mouse_init: couldn't initialize gx mouse");

    g_canvas = gx_canvas_create();
    if (g_canvas == NULL) die("gx_canvas_create: out of memory");

    input_init();

    uint64_t prev_frame = pit_time();

    g_app.button_capacity = 128;
    g_app.buttons = calloc(g_app.button_capacity, sizeof(*g_app.buttons));
    assert(g_app.buttons != NULL);

    g_app.assets.wallpaper = load_image(wallpaper_path, WALLPAPER_COLOR_PALETTE_SIZE);
    gx_palette_set(COLOR__FIRST_COLOR, g_color_palette, COLOR_COUNT);

    int button_y = 1 - 19;
    app_add_button(&g_app, NULL, "/bin/tetris", (gx_Vec2){1, button_y += 19});
    app_add_button(&g_app, NULL, "/bin/pong", (gx_Vec2){1, button_y += 18});
    app_add_button(&g_app, NULL, "/dev/null", (gx_Vec2){1, button_y += 22});
    app_add_button(&g_app, NULL, "/bin/paint", (gx_Vec2){1, button_y += 18});
    app_add_button(&g_app, NULL, "/bin/sh", (gx_Vec2){1, button_y += 18});

    while (1)
    {
        input_handle(&g_app);

        app_update(&g_app);

        app_display(&g_app);

        int delta = pit_time() - prev_frame;
        msleep(MAX(FRAME_DELAY - delta, 0));

        prev_frame = pit_time();
    }
}
