#include "window.h"
#include "memory.h"
#include "io.h"
#include "kmalloc.h"
#include "vga.h"
#include <stdbool.h>
#include <stddef.h>
#include "assert.h"

static Window *g_windows;
static Window *g_focused_window;

Window *window_create(WindowMode mode)
{
    Window *window = kmalloc(sizeof(*window));
    if (!window) return NULL;

    window->mode = mode;

    switch (mode)
    {
            case WINDOW_TEXT:
                window->width = VGA_TEXT_WIDTH_BYTES;
                window->height = VGA_TEXT_HEIGHT;
                break;
            case WINDOW_GRAPHICS:
                window->height = VGA_GRAPHICS_HEIGHT;
                window->width = VGA_GRAPHICS_WIDTH;
                break;
            default:
                assert(false && "Unsupported window mode");
    }

    window->buffer = kcalloc(1, window->height * window->width);

    return window;
}

void window_destroy(Window *window)
{
    kfree(window->buffer);
    kfree(window);
}

Window *window_get_focused_window()
{
    return g_focused_window;
}

static void display_window(Window *window)
{
    assert(window != NULL);

    void *dest = NULL;

    switch (window->mode)
    {
        case WINDOW_TEXT:
            vga_mode_text();
            dest = (void *)VGA_TEXT_ADDRESS;
            break;
        case WINDOW_GRAPHICS:
            vga_mode_graphics();
            dest = (void *)VGA_GRAPHICS_ADDRESS;
            break;
        default:
            assert(false && "Unreachable");
    }

    size_t size = window->width * window->height;
    memmove(dest, window->buffer, size);
}

void window_display_focused_window()
{
    assert(g_focused_window != NULL);
    display_window(g_focused_window);
}

void window_switch_focus_to(Window *window)
{
    assert(window != NULL);
    g_focused_window = window;
    window_display_focused_window();
}

void window_switch_focus()
{
    window_switch_focus_to(window_get_focused_window()->next);
}

void window_register(Window *window)
{
    assert(window != NULL);

    if (g_windows == NULL)
    {
        g_windows = window;
        window->next = window;
        window->prev = window;
        return;
    }

    Window *tail = g_windows->prev;
    Window *head = g_windows;
    window->next = head;
    window->prev = tail;
    tail->next = window;
    head->prev = window;
}

void window_unregister(Window *window)
{
    assert(window != NULL && g_windows != NULL);
    assert(window->next && window->prev);
    assert(window->next->prev == window && window->prev->next == window && "Corrupted window list");

    bool is_only_in_the_list = window->next == window;
    if (is_only_in_the_list)
    {
        assert(window->prev == window && window == g_windows);
        vga_mode_text();
        io_clear_vga();
        puts("Kernel: All windows closed, nothing to display");
        return;
    }

    if (window_get_focused_window() == window)
    {
        window_switch_focus_to(window->next);
    }

    window->prev->next = window->next;
    window->next->prev = window->prev;

    if (window == g_windows)
    {
        g_windows = window->next;
    }

    window->next = NULL;
    window->prev = NULL;
}

bool window_is_in_focus(Window *window)
{
    assert(window != NULL);

    return window == window_get_focused_window();
}

void window_notify_update(Window *window)
{
    assert(window != NULL);

    if (window_is_in_focus(window))
    {
        window_display_focused_window();
    }
}
