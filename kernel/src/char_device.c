#include "char_device.h"
#include "FAT16.h"
#include "assert.h"

static char_device_Descriptor *g_head;

static char_device_Descriptor *find_descriptor(fat16_File *file)
{
    assert(fat16_get_mdscore_flags(file) == fat16_MDSCoreFlags_DEVICE);

    const int major_number = file->file_entry.firstClusterHigh;

    char_device_Descriptor *cur = g_head;

    while (cur)
    {
        if (cur->major_number == major_number)
        {
            return cur;
        }

        cur = cur->next;
    }

    return NULL;
}


size_t char_device_write(fat16_File *file, uint8_t *buffer, uint64_t buffer_size, uint64_t file_offset, bool block)
{
    char_device_Descriptor *desc = find_descriptor(file);

    if (desc == NULL)
    {
        return 0; // TODO: somehow notify of an error
    }

    assert(desc->write);

    const int minor_number = file->file_entry.firstClusterLow;
    return desc->write(buffer, buffer_size, file_offset, minor_number, block);
}

size_t char_device_read(fat16_File *file, uint8_t *buffer, uint64_t buffer_size, uint64_t file_offset, bool block)
{
    char_device_Descriptor *desc = find_descriptor(file);

    if (desc == NULL)
    {
        return 0; // TODO: somehow notify of an error
    }

    assert(desc->read);

    const int minor_number = file->file_entry.firstClusterLow;
    return desc->read(buffer, buffer_size, file_offset, minor_number, block);
}

void char_device_register(char_device_Descriptor *desc)
{
    desc->next = g_head;
    g_head = desc;
}

bool char_device_unregister(int major_number)
{
    if (g_head == NULL)
    {
        return false;
    }

    if (g_head->major_number == major_number)
    {
        g_head = g_head->next;
        return true;
    }

    char_device_Descriptor *prev = g_head;

    while (true)
    {
        assert(prev && "Prev should always be valid");

        if (prev->next == NULL)
        {
            return false;
        }

        if (prev->next->major_number == major_number)
        {
            char_device_Descriptor *const target_desc = prev->next;
            prev->next = target_desc->next;
            target_desc->next = NULL;
            return true;
        }

        prev = prev->next;
    }
}
