#include <gx/gx.h>
#include <gx/screen.h>
#include <stdint.h>

int gx_init()
{
    int ret = gx_screen_mode_graphics();
    if (ret < 0) return ret;

    ret = gx_screen_clear();
    return ret;
}

int gx_deinit()
{
    return gx_screen_mode_text();
}
