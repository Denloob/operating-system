#pragma once

#include <stdint.h>

// @note The 2 LSBits of each channel are ignored.
typedef struct __attribute__((packed))
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
} gx_palette_Color;

/**
 * @brief Set the color palette starting from color at `offset`.
 *
 * @param offset The offset in the palette to start writing from.
 * @param palette The colors to write to the palette.
 * @param length The length of `palette`.
 *
 * @return 0 on success, negative on error
 */
int gx_palette_set(int offset, gx_palette_Color *palette, int length);
