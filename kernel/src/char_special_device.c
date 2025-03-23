#include "char_special_device.h"
#include "FAT16.h"
#include "char_device.h"
#include "io.h"
#include "file.h"
#include "fs.h"
#include "assert.h"
#include "io_keyboard.h"
#include "kmalloc.h"
#include "memory.h"
#include <stdint.h>
#include <stddef.h>
#include "io.h"
#include "pcb.h"
#include "scheduler.h"
#include "vga.h"
#include "window.h"

static size_t handle_write(uint8_t *buffer, uint64_t buffer_size, uint64_t file_offset, int minor_number, bool block);
static size_t handle_read(uint8_t *buffer, uint64_t buffer_size, uint64_t file_offset, int minor_number, bool block);

static void create_char_device_file(const char *path, char_special_device_MinorDeviceType minor_number)
{
    uint16_t parent_cluster;
    FILE file;
    bool success = fat16_create_file(&g_fs_fat16, path, &file.file, &parent_cluster);
    assert(success && "fat16_create_file");

    file.file.file_entry.reserved = fat16_MDSCoreFlags_DEVICE;
    file.file.file_entry.firstClusterHigh = char_special_device_MAJOR_NUMBER;
    file.file.file_entry.firstClusterLow = minor_number;

    success = fat16_update_entry_in_directory(file.file.ref, &file.file.file_entry, parent_cluster);
    assert(success && "fat16_update_root_entry");
}

void char_special_device_init()
{
    static char_device_Descriptor desc = {
        .read = handle_read,
        .write = handle_write,
        .major_number = char_special_device_MAJOR_NUMBER,
    };

    char_device_register(&desc);

    create_char_device_file("/dev/null", char_special_device_MINOR_NULL);
    create_char_device_file("/dev/zero", char_special_device_MINOR_ZERO);
    create_char_device_file("/dev/tty",  char_special_device_MINOR_TTY );
}



#define VGA_COLOR_WHITE 7
void putc_window(Window *win, char ch)
{
    assert(win != NULL && win->mode == WINDOW_TEXT && win->text != NULL);

    size_t window_cells = win->width * win->height;
    uint8_t *buf = (uint8_t *)win->text->buf;

    if (ch == '\b')
    {
        if (win->text->cursor > 0)
        {
            win->text->cursor -= 2;
            buf[win->text->cursor] = ' ';
            buf[win->text->cursor + 1] = VGA_COLOR_WHITE;
        }
        return;
    }

    if (ch == '\n')
    {
        win->text->cursor += win->width;
        win->text->cursor -= win->text->cursor % (win->width);
    }
    else
    {
        if (win->text->cursor + 1 < window_cells)
        {
            buf[win->text->cursor++] = ch;
            buf[win->text->cursor++] = VGA_COLOR_WHITE;
        }
    }

    if (win->text->cursor >= window_cells)
    {
        memmove(buf, buf + win->width, (win->height - 1) * win->width);
        memset(buf + (win->height - 1) * win->width, 0, win->width);
        win->text->cursor = (win->height - 1) * win->width;
    }
}

size_t handle_write(uint8_t *buffer, uint64_t buffer_size, uint64_t file_offset, int minor_number, bool block /* unused */)
{
    switch ((char_special_device_MinorDeviceType)minor_number)
    {
        case char_special_device_MINOR_NULL:
        case char_special_device_MINOR_ZERO:
            return buffer_size;

        case char_special_device_MINOR_TTY:
        {
            PCB *current = scheduler_current_pcb();
            Window *window = PCB_get_window_in_mode(current, WINDOW_TEXT);
            if (window == NULL)
            {
                return -2;
            }

            for (uint64_t i = 0; i < buffer_size; i++)
            {
                putc_window(window, buffer[i]);
            }

            window_notify_update(window);

            return buffer_size;
        }

        default:
            return 0;
    }
}

typedef struct {
    uint8_t *buffer;
    uint64_t buffer_size;
    uint64_t current_index; /* = 0 (initial) */
} TTYReadRefreshArgument;

// @see pcb_IORefresh
static pcb_IORefreshResult pcb_refresh_tty_read(PCB *pcb)
{
    Window *window = PCB_get_window_in_mode(pcb, WINDOW_TEXT);
    if (window == NULL)
    {
        pcb->regs.rax = -2;
        return PCB_IO_REFRESH_DONE;
    }

    if (!window_is_in_focus(window))
    {
        return PCB_IO_REFRESH_CONTINUE;
    }

    if (!io_keyboard_is_key_ready())
    {
        return PCB_IO_REFRESH_CONTINUE;
    }

    TTYReadRefreshArgument *arg = pcb->refresh_arg;
    assert(arg != NULL);
    assert(arg->current_index <= arg->buffer_size);

    char ch = io_keyboard_key_to_char(io_keyboard_wait_key());
    if ((uint8_t)ch == IO_KEY_UNKNOWN)
    {
        return PCB_IO_REFRESH_CONTINUE;
    }

    // syscall checked that the buffer is safe to write to.
    asm volatile ("stac" ::: "memory");
    arg->buffer[arg->current_index] = ch;
    asm volatile ("clac" ::: "memory");
    if (ch == '\b')
    {
        if (arg->current_index > 0)
        {
            putc_window(window, '\b');
            window_notify_update(window);
            arg->current_index--;
        }
        return PCB_IO_REFRESH_CONTINUE;
    }

    putc_window(window, ch); // TODO: add a way to configure the displaying/not-displaying. Maybe it should be a shell feature instead of being a kernel feature.
    window_notify_update(window);

    arg->current_index++;

    if (ch == '\n' || arg->current_index == arg->buffer_size)
    {
        pcb->regs.rax = arg->current_index;
        kfree(arg);
        return PCB_IO_REFRESH_DONE;
    }

    return PCB_IO_REFRESH_CONTINUE;
}

static size_t tty_read_blocking(uint8_t *buffer, uint64_t buffer_size)
{
    PCB *pcb = scheduler_current_pcb();

    TTYReadRefreshArgument *arg = kcalloc(1, sizeof(*arg));
    if (arg == NULL)
    {
        return 0; // Failed
    }

    arg->buffer_size = buffer_size;
    arg->buffer = buffer;

    pcb->refresh_arg = arg;

    scheduler_move_current_process_to_io_queue_and_context_switch(pcb_refresh_tty_read);
}

static size_t tty_read_nonblocking(uint8_t *buffer, uint64_t buffer_size)
{
    assert(io_input_keyboard_key == io_keyboard_wait_key_code && "tty only works with the io_keyboard driver");

    PCB *pcb = scheduler_current_pcb();

    // We are ok with pcb's window (both text and graphics), or parent's text window (but not graphics).
    //  If we allowed parent's graphics, GUIs with multiple windows would "eat" each other's keys.
    Window *window = pcb->window;
    if (window == NULL)
    {
        window = PCB_get_window_in_mode(pcb, WINDOW_TEXT);
    }

    if (window == NULL)
    {
        return -2;
    }

    if (!window_is_in_focus(window))
    {
        return 0;
    }

    for (int i = 0; i < buffer_size; i++)
    {
        if (!io_keyboard_is_key_ready())
        {
            return i;
        }

        // buffer
        uint8_t ch = io_keyboard_key_to_char(io_keyboard_wait_key());
        if (ch == IO_KEY_UNKNOWN)
        {
            i--;
            continue;
        }
        buffer[i] = ch;
    }

    return buffer_size;
}

size_t handle_read(uint8_t *buffer, uint64_t buffer_size, uint64_t file_offset, int minor_number, bool block)
{
    switch ((char_special_device_MinorDeviceType)minor_number)
    {
        case char_special_device_MINOR_NULL:
            return 0; // `null` character device is always empty.
        case char_special_device_MINOR_ZERO:
            memset(buffer, 0, buffer_size);
            return buffer_size;
        case char_special_device_MINOR_TTY:
        {
            if (block)
            {
                tty_read_blocking(buffer, buffer_size);
                assert(false && "Unreachable");
            }
            else
            {
                return tty_read_nonblocking(buffer, buffer_size);
            }
        }
        default:
            return 0;
    }
}
