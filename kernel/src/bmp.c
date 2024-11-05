#include "assert.h"
#include "bmp.h"
#include "memory.h"
#include "vga.h"

void bmp_load_color_palette(const bmp_DIBHeader *dib,
                            const bmp_ColorTableEntry *color_table)
{
    const uint32_t color_palette_size = bmp_get_color_palette_length(dib);

    vga_color_index(0); // Set the color index to the first color in the palette.
    for (int i = 0; i < color_palette_size; i++)
    {
        const bmp_ColorTableEntry *color = &color_table[i];
        vga_color_write_color(color->red, color->green, color->blue);
    }
}

uint32_t bmp_get_color_palette_length(const bmp_DIBHeader *dib)
{
    if (dib->colors_in_use)
        return dib->colors_in_use;

    return 2 << (dib->bits_per_pixel - 1);
}

void bmp_draw_at(int x_offset, int y_offset, const uint8_t *bmp, uint64_t bmp_array_length)
{
    const uint8_t *bmp_end = bmp + bmp_array_length;
#define ASSERT_NO_OVERFLOW(address, size) assert(((uint8_t *)(address) + (size) - 1) < bmp_end && "BMP array size doesn't match header content")

    const bmp_FileHeader *header = (void *)bmp;
    ASSERT_NO_OVERFLOW(header, sizeof(*header));
    assert(header->type == bmp_MAGIC_NUMBER && "Must be a valid bitmap");

    const bmp_DIBHeader *dib_header = (void *)((uint8_t *)header + sizeof(*header));
    ASSERT_NO_OVERFLOW(dib_header, sizeof(*dib_header));
    assert(dib_header->size == sizeof(*dib_header));
    assert(dib_header->width == VGA_GRAPHICS_WIDTH);
    assert(dib_header->height == VGA_GRAPHICS_HEIGHT);
    assert(dib_header->bits_per_pixel == 8 && "Only 8-bit color BMPs are supported"); // 256 colors => 8 bits (Matches VGA mode)
    assert(dib_header->compression == 0 && "Compression not supported");

    const bmp_ColorTableEntry *color_palette = (void *)((uint8_t *)dib_header + sizeof(*dib_header));
    uint64_t palette_length = bmp_get_color_palette_length(dib_header);
    ASSERT_NO_OVERFLOW(color_palette, palette_length);
    bmp_load_color_palette(dib_header, color_palette);

    const uint8_t *pixels = (uint8_t *)color_palette + palette_length * sizeof(bmp_ColorTableEntry);

    const int row_width = (dib_header->width + 3) & ~3; // Calculate row size with (4 byte) padding
    ASSERT_NO_OVERFLOW(pixels, (dib_header->height * row_width));

    const volatile uint8_t *vga_address = VGA_GRAPHICS_ADDRESS;
    for (int y = 0; y < dib_header->height; y++)
    {
        uint8_t vga_y = y_offset + VGA_GRAPHICS_HEIGHT - y - 1; // In BMP, the rows are reversed, aka first row is the last image raw
        memmove((char *)vga_address + vga_y*VGA_GRAPHICS_WIDTH + x_offset, pixels + y*row_width, dib_header->width);
    }
}
