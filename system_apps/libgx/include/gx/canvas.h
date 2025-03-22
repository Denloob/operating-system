#pragma once

#include <stdint.h>

typedef struct {
    int width;
    int height;
    uint8_t buf[];
} gx_Canvas;

/**
 * @brief Allocate a new Canvas. Make sure to destroy it when no longer needed.
 * @see gx_canvas_destroy
 */
gx_Canvas *gx_canvas_create();

/**
 * @brief Allocate a new Canvas of the given size
 *
 * @see gx_canvas_destroy
 */
gx_Canvas *gx_canvas_create_of_size(int width, int height);

/**
 * @brief Destroy the given canvas
 */
void gx_canvas_destroy(gx_Canvas **canvas);

/**
 * @brief Display the given canvas onto the screen
 *
 * @return 0 on success, negative on error
 */
int gx_canvas_draw(const gx_Canvas *canvas);
