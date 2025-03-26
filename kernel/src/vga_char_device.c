#include "vga_char_device.h"
#include "FAT16.h"
#include "file.h"
#include "assert.h"
#include "fs.h"
#include "char_device.h"
#include "math.h"
#include "pcb.h"
#include "vga.h"
#include "io.h"
#include "scheduler.h"
#include <stddef.h>
#include "memory.h"
#include "window.h"

#define VGA_SCREEN_SIZE (VGA_GRAPHICS_WIDTH * VGA_GRAPHICS_HEIGHT)
#define COLOR_PALETTE_SIZE 256

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
            if (file_offset >= VGA_SCREEN_SIZE)
            {
                assert(false && "fseek shouldn't ever allow this to happen");
                return -1;
            }

            const uint64_t available = VGA_SCREEN_SIZE - file_offset;
            const uint64_t size = MIN(buffer_size, available);

            PCB *current_process = scheduler_current_pcb();
            Window *window = PCB_get_window_in_mode(current_process, WINDOW_GRAPHICS);
            if (window == NULL)
            {
                return -2;
            }
            // In any case we copy the new buffer to the window buffer of the process.
            memmove((uint8_t *)window->graphics->buf + file_offset, buffer, size);
            // But if this is the focused process we also copy the buffer to the real vga
            if (window_is_in_focus(window))
            {
                memmove((void *)(VGA_GRAPHICS_ADDRESS + file_offset), buffer, size);
            }
            return size;
        }
        case vga_char_device_MINOR_COLOR_PALETTE:
        {
            if (file_offset >= COLOR_PALETTE_SIZE)
            {
                assert(false && "fseek shouldn't ever allow this to happen");
                return -1;
            }

            PCB *current_process = scheduler_current_pcb();
            Window *window = PCB_get_window_in_mode(current_process, WINDOW_GRAPHICS);
            if (window == NULL)
            {
                return -1;
            }

            const int aligned_buffer_size = math_ALIGN_DOWN(buffer_size, 3);

            memmove(&window->color_palette[file_offset * 3], buffer, aligned_buffer_size);

            if (window_is_in_focus(window))
            {
                const uint64_t palette_length = buffer_size / 3;
                vga_color_palette(file_offset, buffer, palette_length);
            }

            return aligned_buffer_size;
        }
        case vga_char_device_MINOR_CONFIG:
        {
            if (file_offset != 0 || buffer_size != 1)
            {
                assert(file_offset == 0 && "fseek shouldn't ever allow this to happen");
                return -1;
            }

            switch (*buffer)
            {
                case vga_char_device_CONFIG_TEXT:
                    vga_restore_default_color_palette();
                    vga_mode_text();
                    io_clear_vga();
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

static void create_vga_device_file(const char *path, vga_char_device_MinorDeviceType minor_number, size_t file_size)
{
    uint16_t parent_cluster;
    FILE file;
    bool success = fat16_create_file(&g_fs_fat16, path, &file.file, &parent_cluster);
    assert(success && "fat16_create_file");

    file.file.file_entry.reserved = fat16_MDSCoreFlags_DEVICE;
    file.file.file_entry.firstClusterHigh = vga_char_device_MAJOR_NUMBER;
    file.file.file_entry.firstClusterLow = minor_number;
    file.file.file_entry.fileSize = file_size;

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

    create_vga_device_file("/dev/vga/screen", vga_char_device_MINOR_SCREEN, VGA_SCREEN_SIZE);
    create_vga_device_file("/dev/vga/palette", vga_char_device_MINOR_COLOR_PALETTE, COLOR_PALETTE_SIZE);
    create_vga_device_file("/dev/vga/config", vga_char_device_MINOR_CONFIG, 1);
}
