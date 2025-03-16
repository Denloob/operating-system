#include <gx/err.h>
#include "smartptr.h"
#include <gx/palette.h>
#include <stdio.h>

int gx_palette_set(int offset, gx_palette_Color *palette, int length)
{
    FILE *fp = fopen("/dev/vga/palette", "w");
    if (fp == NULL)
    {
        return -gx_err_VGA;
    }
    defer({ fclose(fp); });

    int ret = fseek(fp, offset, SEEK_SET);
    if (ret != 0)
    {
        return -gx_err_VGA;
    }

    int items_written = fwrite(palette, sizeof(*palette), length, fp);
    if (items_written != length)
    {
        return -gx_err_VGA;
    }

    return 0;
}
