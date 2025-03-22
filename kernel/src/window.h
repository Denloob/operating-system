#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum
{
    WINDOW_TEXT,
    WINDOW_GRAPHICS
} WindowMode;

typedef struct {
    size_t cursor;
    uint8_t buf[];
} TextBuffer;

typedef struct {
    uint8_t buf[0];
} GraphicsBuffer;

typedef struct Window
{
    WindowMode mode;
    union {
        GraphicsBuffer *graphics;
        TextBuffer     *text;
    };
    size_t width, height;

    struct Window *next;
    struct Window *prev;
} Window;

// @see window_destroy
Window *window_create(WindowMode mode);

void window_destroy(Window *window);

Window *window_get_focused_window();
bool window_is_in_focus(Window *window);

// NOTE: Must be a registered window
void window_switch_focus_to(Window *window);

/**
 * @brief Switches the focus to the next window after the currently focused one.
 */
void window_switch_focus();

void window_register(Window *window);
void window_unregister(Window *window);

void window_display_focused_window();

/**
 * @brief Notify the given window of an update. If it's in focus, will cause a redraw (aka will display it's new state).
 *          Otherwise, acts as a nop.
 */
void window_notify_update(Window *window);
