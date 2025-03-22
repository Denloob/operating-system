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

#define COLOR_WHITE       0
#define COLOR_BLACK       1
#define COLOR_GRAY        2
#define COLOR_RED         3
#define COLOR_BLUE        4
#define COLOR_GREEN       5
#define COLOR_PURPLE      6
#define COLOR_PINK        7
#define COLOR_COUNT       8

#define COLOR_LIGHT_GRAY  8


gx_palette_Color g_color_palette[256] = {
    [COLOR_WHITE] = {0xff, 0xff, 0xff},
    [COLOR_BLACK] = {0x00, 0x00, 0x00},
    [COLOR_GRAY] = {0x88, 0x88, 0x88},
    [COLOR_LIGHT_GRAY] = {0xcc, 0xcc, 0xcc},
    [COLOR_RED] = {0xff, 0x00, 0x00},
    [COLOR_BLUE] = {0x00, 0x00, 0xff},
    [COLOR_GREEN] = {0x00, 0xff, 0x00},
    [COLOR_PURPLE] = {0xff, 0x00, 0xff},
    [COLOR_PINK] = {0xff, 0xaa, 0xaa},
};

#define COLOR_SELECTION_REGION_END 20

#define PAINT_CANVAS_WIDTH 320
#define PAINT_CANVAS_HEIGHT 180

#define BRUSH_SIZE 4

typedef enum {
    TOOL_BRUSH,
    TOOL_RECT,
    TOOL_CIRCLE,
    TOOL_LINE,
    TOOL_COUNT,
} Tool;

typedef struct {
    gx_Canvas *paint_canvas;
    uint8_t selected_color;

    struct {
        gx_Vec2 pos;
        bool is_pressed;
    } mouse;

    gx_Vec2 prev_mouse_pos;

    Tool selected_tool;
    bool is_drawing;

    gx_Vec2 pressed_moused_pos; // The position at which the mouse was pressed and has been held since. If none, it's -1, -1
} App;


void die(const char *message)
{
    gx_deinit();

    printf("paint: %s\n", message);
    exit(1);
}

/************************
 *        Input
 ************************/

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

/************************
 *      App Logic
 ************************/

gx_Canvas *g_canvas;

bool app_has_pressed_mouse_pos(App *app)
{
    return app->pressed_moused_pos.x != -1;
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

    gx_mouse_State mouse_state = gx_mouse_get_state();

    app->prev_mouse_pos = app->mouse.pos;

    app->mouse.pos.x = mouse_state.x;
    app->mouse.pos.y = mouse_state.y;
    app->mouse.is_pressed = mouse_state.buttons & gx_mouse_BUTTON_LEFT;
}

void draw_filled_square(gx_Canvas *canvas, gx_Vec2 pos, int size, int filling_color)
{
    gx_draw_fill_rect_wh(canvas, pos, size, size, filling_color);
    gx_draw_rect_wh(canvas, pos, size, size, COLOR_BLACK);
}

void draw_color_palette()
{
#define COLOR_PALETTE_X_OFFSET 130
#define COLOR_PALETTE_Y_OFFSET 4
#define COLOR_PALETTE_SQUARE_SIZE 12
#define COLOR_PALETTE_PADDING 10

    int x = COLOR_PALETTE_X_OFFSET;

    for (int i = 0; i < COLOR_COUNT; i++)
    {
        draw_filled_square(g_canvas, (gx_Vec2){x, COLOR_PALETTE_Y_OFFSET}, COLOR_PALETTE_SQUARE_SIZE, i);

        x += COLOR_PALETTE_PADDING + COLOR_PALETTE_SQUARE_SIZE;
    }
}

// -1 if no color, color index otherwise
int position_to_color(gx_Vec2 pos)
{
    if (pos.y < COLOR_PALETTE_Y_OFFSET || pos.y > (COLOR_PALETTE_Y_OFFSET + COLOR_PALETTE_SQUARE_SIZE))
    {
        return -1;
    }

    if (pos.x < COLOR_PALETTE_X_OFFSET)
    {
        return -1;
    }

    pos.x -= COLOR_PALETTE_X_OFFSET;

    for (int i = 0; i < COLOR_COUNT; i++)
    {
        if (pos.x < 0)
        {
            return -1;
        }

        if (pos.x < COLOR_PALETTE_SQUARE_SIZE)
        {
            return i;
        }

        pos.x -= COLOR_PALETTE_PADDING + COLOR_PALETTE_SQUARE_SIZE;
    }

    return -1;
}

void draw_tools_buttons(App *app)
{
#define TOOLS_BUTTONS_X_OFFSET 50
#define TOOLS_BUTTONS_Y_OFFSET 4
#define TOOLS_BUTTONS_SQUARE_SIZE 13
#define TOOLS_BUTTONS_PADDING 4

    int x = TOOLS_BUTTONS_X_OFFSET;
    for (int i = 0; i < TOOL_COUNT; i++)
    {
        if (app->selected_tool == i)
        {
            gx_draw_fill_rect_wh(g_canvas, (gx_Vec2){x, TOOLS_BUTTONS_Y_OFFSET}, TOOLS_BUTTONS_SQUARE_SIZE, TOOLS_BUTTONS_SQUARE_SIZE, COLOR_LIGHT_GRAY);
        }
        gx_draw_rect_wh(g_canvas, (gx_Vec2){x, TOOLS_BUTTONS_Y_OFFSET}, TOOLS_BUTTONS_SQUARE_SIZE, TOOLS_BUTTONS_SQUARE_SIZE, COLOR_BLACK);

        switch (i)
        {
            case TOOL_RECT:
                gx_draw_rect_wh(g_canvas,
                                (gx_Vec2){x + 2, TOOLS_BUTTONS_Y_OFFSET + 4},
                                TOOLS_BUTTONS_SQUARE_SIZE - 4,
                                TOOLS_BUTTONS_SQUARE_SIZE - 8,
                                COLOR_BLACK);
                break;
            case TOOL_CIRCLE:
                gx_draw_circle(g_canvas,
                               (gx_Vec2){x + TOOLS_BUTTONS_SQUARE_SIZE / 2, TOOLS_BUTTONS_Y_OFFSET + TOOLS_BUTTONS_SQUARE_SIZE / 2},
                               TOOLS_BUTTONS_SQUARE_SIZE / 2 - 2,
                               COLOR_BLACK);
                break;
            case TOOL_LINE:
                gx_draw_line(g_canvas,
                             (gx_Vec2){x + 2, TOOLS_BUTTONS_Y_OFFSET + TOOLS_BUTTONS_SQUARE_SIZE - 3},
                             (gx_Vec2){x + TOOLS_BUTTONS_SQUARE_SIZE - 3, TOOLS_BUTTONS_Y_OFFSET + 2},
                             COLOR_BLACK);
                break;
            case TOOL_BRUSH:
                gx_draw_fill_circle(g_canvas,
                                    (gx_Vec2){x + TOOLS_BUTTONS_SQUARE_SIZE / 2, TOOLS_BUTTONS_Y_OFFSET + TOOLS_BUTTONS_SQUARE_SIZE / 2},
                                    2,
                                    COLOR_BLACK);
                break;
            case TOOL_COUNT:
            default:
                assert(false && "Invalid tool");
        }

        x += TOOLS_BUTTONS_PADDING + TOOLS_BUTTONS_SQUARE_SIZE;
    }
}

int position_to_tool(gx_Vec2 pos)
{
    if (pos.y < TOOLS_BUTTONS_Y_OFFSET || pos.y > (TOOLS_BUTTONS_Y_OFFSET + TOOLS_BUTTONS_SQUARE_SIZE))
    {
        return -1;
    }

    if (pos.x < TOOLS_BUTTONS_X_OFFSET)
    {
        return -1;
    }

    pos.x -= TOOLS_BUTTONS_X_OFFSET;

    for (int i = 0; i < TOOL_COUNT; i++)
    {
        if (pos.x < 0)
        {
            return -1;
        }

        if (pos.x < TOOLS_BUTTONS_SQUARE_SIZE)
        {
            return i;
        }

        pos.x -= TOOLS_BUTTONS_PADDING + TOOLS_BUTTONS_SQUARE_SIZE;
    }

    return -1;
}

void draw_trashcan()
{
#define TRASHCAN_X 20
#define TRASHCAN_Y 2
    gx_draw_fill_rect_wh(g_canvas, (gx_Vec2){TRASHCAN_X+4, TRASHCAN_Y}, 4, 1, COLOR_GRAY);
    gx_draw_fill_rect_wh(g_canvas, (gx_Vec2){TRASHCAN_X, TRASHCAN_Y + 1}, 12, 2, COLOR_GRAY);
    gx_draw_fill_rect_wh(g_canvas, (gx_Vec2){TRASHCAN_X+1, TRASHCAN_Y + 3}, 10, 10, COLOR_GRAY);
    gx_draw_rect_wh(g_canvas, (gx_Vec2){TRASHCAN_X+1, TRASHCAN_Y + 3}, 10, 10, COLOR_BLACK);
}

bool is_trashcan_pressed(gx_Vec2 mouse_pos)
{
    return mouse_pos.x >= TRASHCAN_X && mouse_pos.x < (TRASHCAN_X + 12)
            && mouse_pos.y >= 0 && mouse_pos.y < (TRASHCAN_Y + 13);
}

void draw_rect_tool_on(gx_Canvas *canvas, App *app, int y_offset)
{
    assert(app_has_pressed_mouse_pos(app));

    const gx_Vec2 bottom_right = {
        .x = app->pressed_moused_pos.x,
        .y = app->pressed_moused_pos.y - y_offset,
    };

    const gx_Vec2 top_left = {
        .x = app->mouse.pos.x,
        .y = app->mouse.pos.y - y_offset,
    };

    gx_draw_rect(canvas, top_left, bottom_right, app->selected_color);
}
void draw_line_tool_on(gx_Canvas *canvas, App *app, int y_offset)
{
    assert(app_has_pressed_mouse_pos(app));

    const gx_Vec2 bottom_right = {
        .x = app->pressed_moused_pos.x,
        .y = app->pressed_moused_pos.y - y_offset,
    };

    const gx_Vec2 top_left = {
        .x = app->mouse.pos.x,
        .y = app->mouse.pos.y - y_offset,
    };

    gx_draw_line(canvas, top_left, bottom_right, app->selected_color);
}

void draw_circle_tool_on(gx_Canvas *canvas, App *app, int y_offset)
{
    assert(app_has_pressed_mouse_pos(app));

    const gx_Vec2 center = {
        .x = app->pressed_moused_pos.x,
        .y = app->pressed_moused_pos.y - y_offset,
    };

    const gx_Vec2 side = {
        .x = app->mouse.pos.x,
        .y = app->mouse.pos.y - y_offset,
    };

    int dx = center.x - side.x;
    int dy = center.y - side.y;
    int radius = isqrt((dx * dx) + (dy * dy));

    gx_draw_circle(canvas, center, radius, app->selected_tool);
}

/**
 * @brief Update a dipointal tool (tool defined by two points), which can be drawn with `draw_tool_on` func
 */
void app_update_dipointal_tool(App *app, typeof(draw_circle_tool_on) draw_tool_on_func)
{
    bool is_on_canvas = app->mouse.pos.y > COLOR_SELECTION_REGION_END;
    if (!app_has_pressed_mouse_pos(app))
    {
        if (app->mouse.is_pressed && is_on_canvas)
        {
            app->pressed_moused_pos = app->mouse.pos;
        }

        return;
    }

    if (!app->mouse.is_pressed)
    {
        draw_tool_on_func(app->paint_canvas, app, COLOR_SELECTION_REGION_END);
        app->pressed_moused_pos = (gx_Vec2){-1, -1};
        return;
    }
}

void app_update_brush(App *app)
{
    bool is_on_canvas = app->mouse.pos.y > COLOR_SELECTION_REGION_END;
    if (is_on_canvas && app->mouse.is_pressed)
    {
        bool is_prev_on_canvas = app->prev_mouse_pos.y > COLOR_SELECTION_REGION_END;
        if (app->is_drawing && is_prev_on_canvas)
        {
            gx_Vec2 start = {app->prev_mouse_pos.x, app->prev_mouse_pos.y - COLOR_SELECTION_REGION_END};
            gx_Vec2 end = {app->mouse.pos.x, app->mouse.pos.y - COLOR_SELECTION_REGION_END};

            for (int x = 0; x < BRUSH_SIZE; x++)
            {
                for (int y = 0; y < BRUSH_SIZE; y++)
                {
                    gx_draw_line(app->paint_canvas, (gx_Vec2){start.x + x, start.y + y}, (gx_Vec2){end.x + x, end.y + y}, app->selected_color);
                }
            }
        }

        app->is_drawing = true;
    }
    else
    {
        app->is_drawing = false;
    }
}

void app_update(App *app)
{
    switch (app->selected_tool)
    {
        case TOOL_RECT:
            app_update_dipointal_tool(app, draw_rect_tool_on);
            break;
        case TOOL_CIRCLE:
            app_update_dipointal_tool(app, draw_circle_tool_on);
            break;
        case TOOL_LINE:
            app_update_dipointal_tool(app, draw_line_tool_on);
            break;
        case TOOL_BRUSH:
            app_update_brush(app);
            break;
        case TOOL_COUNT:
        default:
            assert(false && "Invalid tool");
    }

    if (app->mouse.is_pressed)
    {
        int selected_color = position_to_color(app->mouse.pos);
        if (selected_color >= 0)
        {
            app->selected_color = selected_color;
        }

        int selected_tool = position_to_tool(app->mouse.pos);
        if (selected_tool >= 0)
        {
            app->selected_tool = selected_tool;
            app->pressed_moused_pos = (gx_Vec2){-1, -1};
        }

        if (is_trashcan_pressed(app->mouse.pos))
        {
            memset(app->paint_canvas->buf, COLOR_WHITE, app->paint_canvas->width * app->paint_canvas->height);
        }
    }
}

void app_display_tools(App *app)
{
    if (app->selected_tool == TOOL_BRUSH)
    {
        return; // Nothing special to display
    }

    if (!app_has_pressed_mouse_pos(app))
    {
        return; // All tools require a pressed_moused_pos
    }

    switch (app->selected_tool)
    {
        case TOOL_RECT:
            draw_rect_tool_on(g_canvas, app, 0);
            break;
        case TOOL_CIRCLE:
            draw_circle_tool_on(g_canvas, app, 0);
            break;
        case TOOL_LINE:
            draw_line_tool_on(g_canvas, app, 0);
            break;
        default:
            assert(false && "Invalid tool");
    }
}

void app_display(App *app)
{
    memset(&g_canvas->buf[0], COLOR_WHITE, COLOR_SELECTION_REGION_END * g_canvas->width);

    gx_draw_canvas(g_canvas, (gx_Vec2){0, COLOR_SELECTION_REGION_END}, app->paint_canvas);

    gx_draw_line(g_canvas, (gx_Vec2){0, COLOR_SELECTION_REGION_END - 1}, (gx_Vec2){g_canvas->width, COLOR_SELECTION_REGION_END - 1}, COLOR_GRAY);

    draw_color_palette();

    draw_trashcan();
    draw_tools_buttons(app);

    if (app->mouse.is_pressed)
    {
        app_display_tools(app);
    }
    else
    {
        gx_draw_fill_rect_wh(g_canvas, app->mouse.pos, BRUSH_SIZE, BRUSH_SIZE, app->selected_color);
    }

    gx_canvas_draw(g_canvas);
}

App g_app;

#define FRAME_DELAY 20

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

    g_app.paint_canvas = gx_canvas_create_of_size(PAINT_CANVAS_WIDTH, PAINT_CANVAS_HEIGHT);
    g_app.selected_color = COLOR_BLACK;
    g_app.pressed_moused_pos = (gx_Vec2){-1, -1};

    uint64_t prev_frame = pit_time();
    while (1)
    {
        input_handle(&g_app);

        app_update(&g_app);

        if (pit_time() - prev_frame > FRAME_DELAY)
        {
            app_display(&g_app);
            prev_frame = pit_time();
        }
    }
}
