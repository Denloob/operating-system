#include <gx/canvas.h>
#include <gx/screen.h>
#include <stdlib.h>
#include "vga.h"

int gx_canvas_draw(const gx_Canvas *canvas)
{
    return gx_screen_write(canvas->buf, canvas->width * canvas->height);
}

void gx_canvas_destroy(gx_Canvas **canvas)
{
    free(canvas);
}

gx_Canvas *gx_canvas_create_of_size(int width, int height)
{
    gx_Canvas *canvas = calloc(1,  sizeof(*canvas) + VGA_GRAPHICS_SIZE);
    canvas->width = width;
    canvas->height = height;

    return canvas;
}

gx_Canvas *gx_canvas_create()
{
    return gx_canvas_create_of_size(VGA_GRAPHICS_WIDTH, VGA_GRAPHICS_HEIGHT);
}
