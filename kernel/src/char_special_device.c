#include "char_special_device.h"
#include "char_device.h"
#include "io.h"
#include "file.h"
#include "fs.h"
#include "assert.h"
#include "memory.h"
#include <stdint.h>
#include <stddef.h>

static size_t handle_write(uint8_t *buffer, uint64_t buffer_size, uint64_t file_offset, int minor_number);
static size_t handle_read(uint8_t *buffer, uint64_t buffer_size, uint64_t file_offset, int minor_number);

static void create_char_device_file(const char *path, char_special_device_MinorDeviceType minor_number)
{
    FILE file;
    bool success = fat16_create_file_with_return(&file.file , &g_fs_fat16, path);
    assert(success && "fat16_create_file");

    file.file.file_entry.reserved = fat16_MDSCoreFlags_DEVICE;
    file.file.file_entry.firstClusterHigh = char_special_device_MAJOR_NUMBER;
    file.file.file_entry.firstClusterLow = minor_number;

    success = fat16_update_root_entry(file.file.ref->drive, &file.file.ref->bpb, &file.file.file_entry);
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

size_t handle_write(uint8_t *buffer, uint64_t buffer_size, uint64_t file_offset, int minor_number)
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

size_t handle_read(uint8_t *buffer, uint64_t buffer_size, uint64_t file_offset, int minor_number)
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
            for (int i = 0; i < buffer_size; i++)
            {
                buffer[i] = key_to_char(io_input_keyboard_key());
                putc(buffer[i]); // TODO: add a way to configure the displaying/not-displaying. Maybe it should be a shell feature instead of being a kernel feature.
                if (buffer[i] == '\n')
                {
                    return i + 1;
                }
            }

            return buffer_size;
        }
        default:
            return 0;
    }
}
