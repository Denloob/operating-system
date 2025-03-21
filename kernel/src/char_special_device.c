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



#define VGA_COLOR_WHITE 0x0F
void putc_window(Window *win, char ch, bool focused)
{
    if (!win || win->mode != WINDOW_TEXT || !win->buffer) return;

    size_t window_cells = win->width * win->height;
    uint8_t *buf = (uint8_t *)win->buffer;
    static size_t write_pos = 0; 

    if (ch == '\b') 
    {
        if (write_pos > 0)
        {
            write_pos -= 2; 
            buf[write_pos] = ' ';
            buf[write_pos + 1] = VGA_COLOR_WHITE;
        }
        return;
    }

    if (ch == '\n') 
    {
        write_pos += win->width;
        write_pos -= write_pos % (win->width);
    }
    else
    {
        if (write_pos + 1 < window_cells)
        {
            buf[write_pos++] = ch;
            buf[write_pos++] = VGA_COLOR_WHITE;
        }
    }

    if (write_pos >= window_cells * 2)
    {
        memmove(buf, buf + win->width, (win->height - 1) * win->width);
        memset(buf + (win->height - 1) * win->width, 0, win->width);
        write_pos = (win->height - 1) * win->width;
    }

    if (focused)
    {
        memmove((void *)io_vga_addr_base, win->buffer, win->width * win->height);
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
            if (vga_current_mode() != VGA_MODE_TYPE_TEXT)
            {
                return -2;
            }

            PCB *current = scheduler_current_pcb();
            if (!current || !current->window || current->window->mode != WINDOW_TEXT)
            {
                return -1;
            }

            bool is_focused = (current == scheduler_foucsed_pcb());

            for (uint64_t i = 0; i < buffer_size; i++)
            {
                putc_window(current->window, buffer[i], is_focused);
            }

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
    PCB *focused = scheduler_foucsed_pcb();
    if(pcb != focused)
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

    char ch = key_to_char(io_input_keyboard_key());

    // syscall checked that the buffer is safe to write to.
    asm volatile ("stac" ::: "memory");
    arg->buffer[arg->current_index] = ch;
    asm volatile ("clac" ::: "memory");
    if (ch == '\b')
    {
        if (arg->current_index > 0)
        {
            putc('\b');
            arg->current_index--;
        }
        return PCB_IO_REFRESH_CONTINUE;
    }

    putc(ch); // TODO: add a way to configure the displaying/not-displaying. Maybe it should be a shell feature instead of being a kernel feature.

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
    assert(io_input_keyboard_key == io_keyboard_wait_key && "tty only works with the io_keyboard driver");
    PCB *pcb = scheduler_current_pcb();
    PCB *focused = scheduler_foucsed_pcb();
    if(pcb!=focused)
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
        buffer[i] = key_to_char(io_keyboard_wait_key());
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
                if (vga_current_mode() != VGA_MODE_TYPE_TEXT)
                {
                    return -2;
                }

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
