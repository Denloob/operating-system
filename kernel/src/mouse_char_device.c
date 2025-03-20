#include "mouse_char_device.h"
#include "FAT16.h"
#include "file.h"
#include "assert.h"
#include "fs.h"
#include "char_device.h"
#include "math.h"
#include "mouse.h"
#include "memory.h"

typedef struct __attribute__((packed)) {
    int16_t x;
    int16_t y;
    uint8_t buttons;
} MouseData;

static size_t handle_write(uint8_t *buffer, uint64_t buffer_size, uint64_t file_offset, int minor_number, bool __attribute__((unused)) block)
{
    // Mouse device is read-only
    return -1;
}

static size_t handle_read(uint8_t *buffer, uint64_t buffer_size, uint64_t file_offset, int minor_number, bool block)
{
    if (buffer_size != sizeof(MouseData))
    {
        return -1;
    }

    switch (minor_number)
    {
        case mouse_char_device_MINOR_MOUSE:
        {
            MouseData data = {
                .x = mouse_get_x(),
                .y = mouse_get_y(),
                .buttons = mouse_get_buttons()
            };

            assert(buffer_size == sizeof(data));
            memmove(buffer, &data, sizeof(data));

            return buffer_size;
        }
        default:
            return -1;
    }
}

static void create_mouse_device_file(const char *path, mouse_char_device_MinorDeviceType minor_number, size_t file_size)
{
    uint16_t parent_cluster;
    FILE file;
    bool success = fat16_create_file(&g_fs_fat16, path, &file.file, &parent_cluster);
    assert(success && "fat16_create_file");

    file.file.file_entry.reserved = fat16_MDSCoreFlags_DEVICE;
    file.file.file_entry.firstClusterHigh = mouse_char_device_MAJOR_NUMBER;
    file.file.file_entry.firstClusterLow = minor_number;
    file.file.file_entry.fileSize = file_size;

    success = fat16_update_entry_in_directory(file.file.ref, &file.file.file_entry, parent_cluster);
    assert(success && "fat16_update_root_entry");
}

void mouse_char_device_init()
{
    static char_device_Descriptor desc = {
        .read = handle_read,
        .write = handle_write,
        .major_number = mouse_char_device_MAJOR_NUMBER,
    };

    char_device_register(&desc);

    res rs = fat16_create_directory(&g_fs_fat16, "input", "/dev");
    assert(IS_OK(rs) && "fat16_create_directory");

    create_mouse_device_file("/dev/input/mouse", mouse_char_device_MINOR_MOUSE, sizeof(MouseData));
}
