#include "char_special_device.h"
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
#include "scheduler.h"

static size_t handle_write(uint8_t *buffer, uint64_t buffer_size, uint64_t file_offset, int minor_number, bool block);
static size_t handle_read(uint8_t *buffer, uint64_t buffer_size, uint64_t file_offset, int minor_number, bool block);

static void create_char_device_file(const char *path, char_special_device_MinorDeviceType minor_number)
{
    FILE file;
    bool success = fat16_create_file_with_return(&file.file , &g_fs_fat16, path);
    assert(success && "fat16_create_file");

    file.file.file_entry.reserved = fat16_MDSCoreFlags_DEVICE;
    file.file.file_entry.firstClusterHigh = char_special_device_MAJOR_NUMBER;
    file.file.file_entry.firstClusterLow = minor_number;

    success = fat16_update_root_entry(file.file.ref, &file.file.file_entry);
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

    create_char_device_file("null.dev", char_special_device_MINOR_NULL);
    create_char_device_file("zero.dev", char_special_device_MINOR_ZERO);
    create_char_device_file("tty.dev",  char_special_device_MINOR_TTY );
}

size_t handle_write(uint8_t *buffer, uint64_t buffer_size, uint64_t file_offset, int minor_number, bool block /* unused */)
{
    switch ((char_special_device_MinorDeviceType)minor_number)
    {
        case char_special_device_MINOR_NULL:
        case char_special_device_MINOR_ZERO:
            return buffer_size; // Discard data written to `zero` and `null`.
        case char_special_device_MINOR_TTY:
        {
            for (int i = 0; i < buffer_size; i++)
            {
                putc(buffer[i]);
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
    for (int i = 0; i < buffer_size; i++)
    {
        // buffer
        buffer[i] = key_to_char(io_input_keyboard_key());
        if (buffer[i] == '\b')
        {
            if (i > 0)
            {
                putc('\b');
                i--;
            }
            i--;
            continue;
        }

        putc(buffer[i]); // TODO: add a way to configure the displaying/not-displaying. Maybe it should be a shell feature instead of being a kernel feature.

        if (buffer[i] == '\n')
        {
            return i + 1;
        }
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
            }
            else
            {
                tty_read_nonblocking(buffer, buffer_size);
            }
        }
        default:
            return 0;
    }
}
