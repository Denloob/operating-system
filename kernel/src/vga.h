#pragma once

#include <stdint.h>

#define VGA_GRAPHICS_BUF1_PHYS 0xa0000
#define VGA_GRAPHICS_BUF3_PHYS 0xb8000
#define VGA_GRAPHICS_BUF3_PHYS_END 0xb8fa0
#define VGA_GRAPHICS_BUF1 0xffff8000000a0000
#define VGA_GRAPHICS_BUF2 0xffff8000000b0000
#define VGA_GRAPHICS_BUF3 0xffff8000000b8000

#define VGA_GRAPHICS_ADDRESS (volatile char *)VGA_GRAPHICS_BUF1
#define VGA_GRAPHICS_WIDTH 320
#define VGA_GRAPHICS_HEIGHT 200
#define VGA_TEXT_ADDRESS (volatile char *)VGA_GRAPHICS_BUF3
#define VGA_TEXT_WIDTH 80
#define VGA_TEXT_WIDTH_BYTES (VGA_TEXT_WIDTH * 2)
#define VGA_TEXT_HEIGHT 25

void vga_mode_graphics();
void vga_mode_text();

/**
 * @brief - Set the VGA color palette, starting from color index `color_index`.
 *          The palette is an array of {R1,G1,B1,R2,G2,B2};
 *
 * @param color_index    - The index of the first color in the palette
 * @param palette        - `palette_length` colors (`palette_length*3` elements) starting from the index. NOTE: Only the lowest 6 bits will be used.
 * @param palette_length - The amount of colors (not elements) in the palette.
 */
void vga_color_palette(uint8_t color_index, const uint8_t *palette, uint32_t palette_length);

/**
 * @brief - Set a color at color_index to the given color.
 *
 * @note - only 6 bits of each color are used.
 */
void vga_color(uint8_t color_index, uint8_t red, uint8_t green, uint8_t blue);

/**
 * @brief - Sets the index for color writing. Should be chained with vga_color_write_color.
 *
 * @param color_index -The index to set.
 * @see - vga_color_write_color
 */
void vga_color_index(uint8_t color_index);

/**
 * @brief - Writes the 3 given colors to the current color index. Set the color
 *          index with `vga_color_index` and then call this function as many
 *          times as needed, as index is auto incremented.
 *
 * @warning - Before calling this function call vga_color_index
 * @see - vga_color_index
 */
void vga_color_write_color(uint8_t red, uint8_t green, uint8_t blue);
