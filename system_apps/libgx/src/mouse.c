#include <gx/mouse.h>
#include <gx/err.h>
#include <assert.h>
#include <stdio.h>

static FILE *g_mouse_fp;

int gx_mouse_init()
{
    assert(g_mouse_fp == NULL);

    g_mouse_fp = fopen("/dev/input/mouse", "r");
    if (g_mouse_fp == NULL) return -gx_err_MOUSE;

    return 0;
}

gx_mouse_State gx_mouse_get_state()
{
    assert(g_mouse_fp != NULL && "Initialize first");

    gx_mouse_State mouse_pos = {};

    int items_read = fread(&mouse_pos, sizeof(mouse_pos), 1, g_mouse_fp);
    assert(items_read == 1 && "Should never fail");

    return mouse_pos;
}
