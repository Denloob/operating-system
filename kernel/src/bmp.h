#pragma once

#include "file.h"
#include <stdint.h>

#define bmp_MAGIC_NUMBER 0x4d42 /* The string BM ('MB' because little endian) */

typedef struct
{
    uint16_t type;          // File type; must be 'BM'
    uint32_t size;          // Size of the file in bytes
    uint32_t reserved;      // Reserved; must be 0
    uint32_t bfOffBits;     // Offset to start of pixel data
} __attribute__((packed)) bmp_FileHeader;

typedef struct
{
    uint32_t size;                // Size of this header (40 bytes)
    int32_t  width;               // Width of the bitmap in pixels
    int32_t  height;              // Height of the bitmap in pixels
    uint16_t color_planes;        // Number of color planes (must be 1)
    uint16_t bits_per_pixel;      // Bits per pixel (should be 8 for 256 colors)
    uint32_t compression;         // Compression type (0 = no compression)
    uint32_t image_size;          // Size of the image data (can be 0 for uncompressed)
    int32_t  x_pixels_per_meter;  // Horizontal resolution (pixels per meter)
    int32_t  y_pixels_per_meter;  // Vertical resolution (pixels per meter)
    uint32_t colors_in_use;       // Number of colors in the palette
    uint32_t important_colors;    // Important colors (0 = all are important)
} __attribute__((packed)) bmp_DIBHeader;

typedef struct
{
    uint8_t blue;        // Blue value
    uint8_t green;       // Green value
    uint8_t red;         // Red value
    uint8_t reserved;    // Reserved; must be 0
} __attribute__((packed)) bmp_ColorTableEntry;

/**
 * @brief - Load the color table in `color_table` described in `dib` into VGA color palette.
 *
 * @param dib         - The DIB corresponding to the color table.
 * @param color_table - The color table to load
 */
void bmp_load_color_palette(const bmp_DIBHeader *dib,
                            const bmp_ColorTableEntry *color_table);

/**
 * @brief - Get the length of the color palette described in the `dib`.
 *
 * @param dib - The dib
 * @return - The length of the color table. To get the size in bytes, multiply the
 *              length by sizeof(bmp_ColorTableEntry).
 */
uint32_t bmp_get_color_palette_length(const bmp_DIBHeader *dib);

/**
 * @brief - Draw the given bmp at `x` and `y` in VGA.
 * @note - VGA must already be in graphics mode.
 *
 * @param x   - Horizontal offset of the image from top left of VGA.
 * @param y   - Vertical offset of the image form top left of VGA.
 * @param bmp - The BMP image to draw.
 * @param bmp_array_length - The length of the `bmp` in RAM.
 */
void bmp_draw_at(int x, int y, const uint8_t *bmp, uint64_t bmp_array_length);

/**
 * @brief - Draw the bmp in the given file at `x` and `y` in VGA.
 * @note - VGA must already be in graphics mode.
 *
 * @param x    - Horizontal offset of the image from top left of VGA.
 * @param y    - Vertical offset of the image form top left of VGA.
 * @param file - The file of the bmp to draw
 */
void bmp_draw_from_stream_at(int x_offset, int y_offset, FILE *file);
