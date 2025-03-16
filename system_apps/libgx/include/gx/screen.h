#pragma once

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Set the screen mode to text mode
 *
 * @see gx_screen_mode_graphics
 *
 * @return 0 on success, negative on error
 */
int gx_screen_mode_text();

/**
 * @brief Set the screen mode to graphics mode. Required for the lib graphics to work.
 * @note Make sure to set back to text after you are done. Do not output text when in graphics mode.
 *
 * @see gx_screen_mode_text
 *
 * @return 0 on success, negative on error
 */
int gx_screen_mode_graphics();

/**
 * @brief Clear out the screen.
 *
 * @return 0 on success, negative on error
 */
int gx_screen_clear();

/**
 * @brief Write the given buffer of size `size` directly to the screen.
 *
 * @return 0 on success, negative on error
 */
int gx_screen_write(const uint8_t *buf, size_t size);
