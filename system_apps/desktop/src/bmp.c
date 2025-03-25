#include "bmp.h"
#include <string.h>
#include <math.h>
#include <assert.h>
#include <gx/gx.h>
#include <gx/vec.h>
#include <gx/palette.h>
#include <stdlib.h>

void bmp_load_color_palette(const bmp_DIBHeader *dib,
                            const bmp_ColorTableEntry *color_table, int color_offset, int color_palette_limit)
{
    const uint32_t color_palette_size = MIN(color_palette_limit, bmp_get_color_palette_length(dib));

    gx_palette_Color *palette = malloc(color_palette_size * sizeof(*palette));

    for (int i = 0; i < color_palette_size; i++)
    {
        const bmp_ColorTableEntry *color = &color_table[i];
        palette[i] = (gx_palette_Color){
            color->red,
            color->green,
            color->blue,
        };
    }

    gx_palette_set(color_offset, palette, color_palette_size);

    free(palette);
}

uint32_t bmp_get_color_palette_length(const bmp_DIBHeader *dib)
{
    if (dib->colors_in_use)
        return dib->colors_in_use;

    return 2 << (dib->bits_per_pixel - 1);
}

void copy_image_to_canvas(const uint8_t *pixels, const bmp_DIBHeader *dib_header, gx_Canvas *canvas, gx_Vec2 pos, int color_offset)
{
    // Find the visible region on the canvas
    int start_x = MAX(0, pos.x);
    int end_x = MIN(canvas->width, pos.x + dib_header->width);
    int start_y = MAX(0, pos.y);
    int end_y = MIN(canvas->height, pos.y + dib_header->height);

    if (start_x >= end_x || start_y >= end_y)
    {
        return;
    }

    // Find the matching image pixel offsets
    int img_start_x = start_x - pos.x;
    int img_width = end_x - start_x;
    int img_start_y = dib_header->height - (end_y - pos.y);
    int row_width = math_ALIGN_UP(dib_header->width, 4);

    for (int canvas_y = start_y; canvas_y < end_y; canvas_y++)
    {
        int img_y = dib_header->height - (img_start_y + (canvas_y - start_y)) - 1; // In BMP, the rows are reversed, aka first row is the last image raw
        if (img_y < 0 || img_y >= dib_header->height)
        {
            continue; // Should be unecessary, but better safe than sorry
        }

        const uint8_t *src = pixels + (img_y * row_width) + img_start_x;
        uint8_t *dst = canvas->buf + (canvas_y * canvas->width) + start_x;

        memmove(dst, src, img_width);
        for (size_t i = 0; i < img_width; i++)
        {
            dst[i] = src[i] + color_offset;
        }
    }
}

void bmp_draw_at(gx_Canvas *canvas, gx_Vec2 pos, const uint8_t *bmp, uint64_t bmp_array_length, int color_offset, int color_palette_limit)
{
    if (pos.x > canvas->height || pos.y > canvas->height)
    {
        return;
    }

    const uint8_t *bmp_end = bmp + bmp_array_length;
#define ASSERT_NO_OVERFLOW(address, size) assert(((uint8_t *)(address) + (size) - 1) < bmp_end && "BMP array size doesn't match header content")

    const bmp_DIBHeader *dib_header = bmp_get_dib_header(bmp, bmp_array_length);

    const bmp_ColorTableEntry *color_palette = (void *)((uint8_t *)dib_header + sizeof(*dib_header));
    uint64_t palette_length = bmp_get_color_palette_length(dib_header);
    ASSERT_NO_OVERFLOW(color_palette, palette_length);
    bmp_load_color_palette(dib_header, color_palette, color_offset, color_palette_limit);

    const uint8_t *pixels = (uint8_t *)color_palette + palette_length * sizeof(bmp_ColorTableEntry);

    const int row_width = math_ALIGN_UP(dib_header->width, 4); // Calculate row size with (4 byte) padding
    ASSERT_NO_OVERFLOW(pixels, (dib_header->height * row_width));

    copy_image_to_canvas(pixels, dib_header, canvas, pos, color_offset);
}

const bmp_DIBHeader *bmp_get_dib_header(const uint8_t *bmp, uint64_t bmp_array_length)
{
    const bmp_FileHeader *header = (void *)bmp;
    const uint8_t *bmp_end = bmp + bmp_array_length;

    ASSERT_NO_OVERFLOW(header, sizeof(*header));
    assert(header->type == bmp_MAGIC_NUMBER && "Must be a valid bitmap");

    const bmp_DIBHeader *dib_header = (void *)((uint8_t *)header + sizeof(*header));
    ASSERT_NO_OVERFLOW(dib_header, sizeof(*dib_header));
    assert(dib_header->size == sizeof(*dib_header));
    assert(dib_header->bits_per_pixel == 8 && "Only 8-bit color BMPs are supported"); // 256 colors => 8 bits (Matches VGA mode)
    assert(dib_header->compression == 0 && "Compression not supported");
    assert(dib_header->height > 0 && dib_header->width > 0);

    return dib_header;
}
