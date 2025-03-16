#pragma once

#include <gx/canvas.h>
#include <gx/vec.h>

/**
 * @brief An index into the color palette.
 */
typedef uint8_t gx_Color;

/**
 * @brief Draw a single point on the canvas.
 *
 * @param point The position of the point.
 * @param color The color to draw the point in.
 * @note Content outside the canvas bounds will not be drawn.
 */
void gx_draw_point(gx_Canvas* canvas, gx_Vec2 point, gx_Color color);

/**
 * @brief Draw a line on the canvas between two points.
 *
 * @param start The starting point of the line.
 * @param end The ending point of the line.
 * @param color The color to draw the line in.
 * @note Content outside the canvas bounds will not be drawn.
 */
void gx_draw_line(gx_Canvas* canvas, gx_Vec2 start, gx_Vec2 end, gx_Color color);

/**
 * @brief Draw the outline of a rectangle on the canvas.
 *
 * @param top_left The top-left corner of the rectangle.
 * @param bottom_right The bottom-right corner of the rectangle.
 * @param color The color to draw the rectangle outline in.
 * @note Content outside the canvas bounds will not be drawn. Assumes top_left.x <= bottom_right.x and top_left.y <= bottom_right.y.
 */
void gx_draw_rect(gx_Canvas* canvas, gx_Vec2 top_left, gx_Vec2 bottom_right, gx_Color color);

/**
 * @brief Draw a filled rectangle on the canvas.
 *
 * @param top_left The top-left corner of the rectangle.
 * @param bottom_right The bottom-right corner of the rectangle.
 * @param color The color to fill the rectangle with.
 * @note Content outside the canvas bounds will not be drawn. Assumes top_left.x <= bottom_right.x and top_left.y <= bottom_right.y.
 */
void gx_draw_fill_rect(gx_Canvas* canvas, gx_Vec2 top_left, gx_Vec2 bottom_right, gx_Color color);

/**
 * @brief Draw the outline of a circle on the canvas.
 *
 * @param center The center of the circle.
 * @param radius The radius of the circle.
 * @param color The color to draw the circle outline in.
 * @note Content outside the canvas bounds will not be drawn. Assumes radius >= 0.
 */
void gx_draw_circle(gx_Canvas* canvas, gx_Vec2 center, int radius, gx_Color color);

/**
 * @brief Draw a filled circle on the canvas.
 *
 * @param center The center of the circle.
 * @param radius The radius of the circle.
 * @param color The color to fill the circle with.
 * @note Content outside the canvas bounds will not be drawn. Assumes radius >= 0.
 */
void gx_draw_fill_circle(gx_Canvas* canvas, gx_Vec2 center, int radius, gx_Color color);
