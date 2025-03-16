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

gx_Canvas *gx_canvas_create()
{
    gx_Canvas *canvas = calloc(1,  sizeof(*canvas) + VGA_GRAPHICS_SIZE);
    canvas->width = VGA_GRAPHICS_WIDTH;
    canvas->height = VGA_GRAPHICS_HEIGHT;

    return canvas;
}
