#include "vga_char_device.h"
#include "FAT16.h"
#include "file.h"
#include "assert.h"
#include "fs.h"
#include "char_device.h"
#include "math.h"
#include "vga.h"
#include <stddef.h>
#include "memory.h"

static size_t handle_write(uint8_t *buffer, uint64_t buffer_size, uint64_t file_offset, int minor_number, bool __attribute__((unused)) block)
{
    if (buffer_size == 0)
    {
        return 0;
    }

    switch (minor_number)
    {
        case vga_char_device_MINOR_SCREEN:
        {
            if (vga_current_mode() != VGA_MODE_TYPE_GRAPHICS)
            {
                return -2;
            }

            const uint64_t screen_size = VGA_GRAPHICS_WIDTH * VGA_GRAPHICS_HEIGHT;
            if (file_offset >= screen_size)
            {
                return -1;
            }

            const uint64_t available = screen_size - file_offset;

            const uint64_t size = MIN(buffer_size, available);
            memmove((void *)(VGA_GRAPHICS_ADDRESS + file_offset), buffer, size);
            return size;
        }
        case vga_char_device_MINOR_COLOR_PALETTE:
        {
            if (vga_current_mode() != VGA_MODE_TYPE_GRAPHICS)
            {
                return -2;
            }

            if (file_offset > UINT8_MAX)
            {
                return -1;
            }

            const uint64_t palette_length = buffer_size / 3;
            vga_color_palette(file_offset, buffer, palette_length);

            return palette_length * 3;
        }
        case vga_char_device_MINOR_CONFIG:
        {
            if (file_offset != 0 || buffer_size != 1)
            {
                return -1;
            }

            switch (*buffer)
            {
                case vga_char_device_CONFIG_TEXT:
                    vga_mode_text();
                    break;
                case vga_char_device_CONFIG_GRAPHICS:
                    vga_mode_graphics();
                    break;
                default:
                    return -1;
            }

            return 1;
        }
        default:
            return -1;
    }
}

static size_t handle_read(uint8_t *buffer, uint64_t buffer_size, uint64_t file_offset, int minor_number, bool block)
{
    if (buffer_size == 0)
    {
        return 0;
    }

    switch (minor_number)
    {
        case vga_char_device_MINOR_CONFIG:
        {
            switch (vga_current_mode())
            {
                case VGA_MODE_TYPE_GRAPHICS:
                    *buffer = vga_char_device_CONFIG_GRAPHICS;
                case VGA_MODE_TYPE_TEXT:
                    *buffer = vga_char_device_CONFIG_TEXT;
                default:
                    assert(false && "Unknown vga mode");
            }
            return 1;
        }
        default: // None of other devices support read
            return -1;
    }
}

static void create_vga_device_file(const char *path, vga_char_device_MinorDeviceType minor_number)
{
    uint16_t parent_cluster;
    FILE file;
    bool success = fat16_create_file(&g_fs_fat16, path, &file.file, &parent_cluster);
    assert(success && "fat16_create_file");

    file.file.file_entry.reserved = fat16_MDSCoreFlags_DEVICE;
    file.file.file_entry.firstClusterHigh = vga_char_device_MAJOR_NUMBER;
    file.file.file_entry.firstClusterLow = minor_number;

    success = fat16_update_entry_in_directory(file.file.ref, &file.file.file_entry, parent_cluster);
    assert(success && "fat16_update_root_entry");
}

void vga_char_device_init()
{
    static char_device_Descriptor desc = {
        .read = handle_read,
        .write = handle_write,
        .major_number = vga_char_device_MAJOR_NUMBER,
    };

    char_device_register(&desc);

    res rs = fat16_create_directory(&g_fs_fat16, "vga", "/dev");
    assert(IS_OK(rs) && "fat16_create_directory");

    create_vga_device_file("/dev/vga/screen", vga_char_device_MINOR_SCREEN);
    create_vga_device_file("/dev/vga/palette", vga_char_device_MINOR_COLOR_PALETTE);
    create_vga_device_file("/dev/vga/config", vga_char_device_MINOR_CONFIG);
}
