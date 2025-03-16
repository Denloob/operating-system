#pragma once

#include <gx/canvas.h>

/**
 * @brief Deinitialize libgx.
 *
 * @see gx_init
 *
 * @return 0 on success, negative on error
 */
int gx_deinit();

/**
 * @brief Initialize libgx for use. Make sure to call gx_deinit before exiting or writing to stdout
 *
 * @see gx_deinit
 *
 * @return 0 on success, negative on error
 */
int gx_init();
