#include "smartptr.h"
#include <assert.h>
#include <gx/screen.h>
#include <gx/err.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "vga.h"

enum {
    vga_char_device_CONFIG_TEXT     = '0',
    vga_char_device_CONFIG_GRAPHICS = '1',
};

int gx_screen_mode_text()
{
    FILE *fp = fopen("/dev/vga/config", "w");
    if (fp == NULL) return -gx_err_VGA;
    defer({ fclose(fp); });

    uint8_t mode = vga_char_device_CONFIG_TEXT;
    size_t items_written = fwrite(&mode, sizeof(mode), 1, fp);
    if (items_written != 1) return -gx_err_VGA;

    return 0;
}

int gx_screen_mode_graphics()
{
    FILE *fp = fopen("/dev/vga/config", "w");
    if (fp == NULL) return -gx_err_VGA;
    defer({ fclose(fp); });

    uint8_t mode = vga_char_device_CONFIG_GRAPHICS;
    size_t items_written = fwrite(&mode, sizeof(mode), 1, fp);
    if (items_written != 1) return -gx_err_VGA;

    return 0;
}

static FILE *g_screen_fp;

static int g_screen_fp_init()
{
    assert(g_screen_fp == NULL);

    g_screen_fp = fopen("/dev/vga/screen", "w");
    if (g_screen_fp == NULL) return -gx_err_VGA;

    return 0;
}

int gx_screen_clear()
{
    uint8_t *buf = calloc(1, VGA_GRAPHICS_SIZE);
    int ret = gx_screen_write(buf, VGA_GRAPHICS_SIZE);
    free(buf);

    return ret;
}

int gx_screen_write(const uint8_t *buf, size_t size)
{
    if (g_screen_fp == NULL)
    {
        int ret = g_screen_fp_init();
        if (ret < 0) return ret;
    }
    else
    {
        rewind(g_screen_fp);
    }

    size_t items_written = fwrite(buf, size, 1, g_screen_fp);
    if (items_written != 1) return -gx_err_VGA;

    return 0;
}
